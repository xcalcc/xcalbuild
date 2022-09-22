/*
   Copyright (C) 2019-2022 Xcalibyte (Shenzhen) Limited.
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
     http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "command-line.hpp"
#include "build-processor.hpp"
#include "toolchain-profile.hpp"
#include "tracer.hpp"
#include "logger.hpp"
#include "error-code.hpp"

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

using namespace xcal;

int main(int argc, char *argv[]) try {
    // Parse command line.
    CommandLine command_line;

#ifndef _WIN32
    setenv("LC_ALL", "C", true);
#endif // !_WIN32

    // parse command line parameters before going further for preprocess
    auto ec_opt = command_line.parse(argc, argv);
    if(ec_opt) {
        // Terminate after parsing the arg.
        return EXIT_CODE(*ec_opt);
    }
    ec_opt = command_line.parse_config();
    if(ec_opt) {
        return EXIT_CODE(*ec_opt);
    }

    // configure logging elements when command line check is fine
    auto log_file = (fs::system_complete(command_line.get_output_dir()) / fs::path("xcalbuild.log")).string();
    init_logger(log_file, command_line.get_trace_id(), command_line.get_span_id(), command_line.is_debug(), command_line.is_local_log());

    LOG(info) << "Build dir: " << command_line.get_build_dir();
    LOG(info) << "Output dir: " << command_line.get_output_dir();
    LOG(info) << "Prebuild command: " << command_line.get_prebuild();
    LOG(info) << "Build command: " << ba::join(command_line.get_build_commands(), " ");
    LOG(info) << "Trace ID: " << command_line.get_trace_id();
    LOG(info) << "Span ID: " << command_line.get_span_id();
    LOG(info) << "Logging to local file: " << command_line.is_local_log();

    // Load toolchain profile as specified.
    LOG(info) << "Load toolchain profile as specified";
    ToolchainProfile profile;
    ec_opt = profile.load(command_line.get_toolchain_profile());
    if(ec_opt) {
        // Error loading the profile.
        return EXIT_CODE(*ec_opt);
    }

    // Trace native build according to probe result, and generate build spec.
    LOG(info) << "Trace native build according to probe result, and generate build spec";
    Tracer tracer(command_line.get_tracing_method(), command_line, profile);
    ec_opt = tracer.trace();
    //GCOVR_EXCL_START for now this only happens when strace is not installed.
    if(ec_opt) {
        // Error tracing the build.
        return EXIT_CODE(*ec_opt);
    }
    //GCOVR_EXCL_STOP

    // Process the build spec.
    BuildProcessor bp(profile, command_line, command_line.get_parallelism());
    ec_opt = bp.process();
    //GCOVR_EXCL_START
    if(ec_opt) {
        return EXIT_CODE(*ec_opt);
    }
    //GCOVR_EXCL_STOP

    return EXIT_CODE(EC_NONE);
//GCOVR_EXCL_START
} catch(const std::exception& e) {
    LOG_EC(error, EC_UNKOWN) << "Unkown exception. " << e.what();
    return EXIT_CODE(EC_UNKOWN);
}
//GCOVR_EXCL_STOP