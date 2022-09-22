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
#include <boost/process.hpp>
#include <boost/process/env.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <iostream>

#include <nlohmann/json.hpp>

#include "command-line.hpp"
#include "toolchain-profile.hpp"
#include "tracer.hpp"
#include "logger.hpp"
#include "profiler.hpp"

namespace bp = boost::process;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

using json = nlohmann::json;

using namespace xcal;

Tracer::Tracer(
    TracingMethod tracing_method,
    const CommandLine &command_line,
    const ToolchainProfile &toolchain_profile
):
    tracing_method(tracing_method),
    build_dir_path(fs::canonical(fs::system_complete(command_line.get_build_dir()))),
    compile_db_path(fs::canonical(fs::system_complete(command_line.get_output_dir())) /
                    command_line.get_cdb_name()),
    tool_root(command_line.get_tool_root()),
    prebuild(command_line.get_prebuild()),
    build_commands(command_line.get_build_commands()),
    binaries(toolchain_profile.get_binaries_to_trace())
{}

int Tracer::trace_with_bear() {
    #ifdef __APPLE__
        auto preinstall_tracer_path = bp::search_path("bear");
        if(!preinstall_tracer_path.string().length()) {
            LOG_EC(critical, EC_STRACE_NOT_FOUND) << "`bear` not found, please install it, or choose a different tracing method.";
            return EC_BEAR_NOT_FOUND;
        }

        // Run the build command with preinstalled tracer.
        LOG(info) << "Run the build command with preinstalled tracer in child process";
        int ec = 0;
        try {
            bp::child b(
                preinstall_tracer_path, "--output", compile_db_path, "--",
                bp::args(build_commands),
                bp::start_dir=build_dir_path);
            b.wait();
            ec = b.exit_code();
        } catch (boost::process::process_error &err) {
            LOG(warning) << "Failed to run " << preinstall_tracer_path << ", reason:" << err.what();
            ec = 1;
        }
        return ec;
    #else
        auto build_tracer_path = tool_root / "bin" / fs::path("unix-tracer");
        // Libear path need to be absolute.
        // Add both 32/64 path to LD_LIBRARY_PATH so that it won't generate "Error ..., ignored".
        auto libear_path = (tool_root / "lib32").string() + ":" + (tool_root / "lib64").string();

        // Add overriding binary list if any.
        std::vector<std::string> additional_args;
        if(binaries.size()) {
            std::vector<std::string> names;
            for(const auto &p: binaries) {
                names.push_back(p.first);
            }
            additional_args.push_back("-c");
            additional_args.push_back(ba::join(names, ","));
        }

        // Run the build command with build tracer.
        LOG(info) << "Run the build command with build tracer in child process";
        int ec = 0;
        try {
            bp::child b(
                build_tracer_path, "-l", libear_path, "-o", compile_db_path,
                bp::args(additional_args), "--",
                bp::args(build_commands),
                bp::start_dir=build_dir_path);
            b.wait();
            ec = b.exit_code();
        //GCOVR_EXCL_START
        } catch (boost::process::process_error &err) {
            LOG(warning) << "Failed to run " << build_tracer_path << ", reason:" << err.what();
            ec = 1;
        }
        //GCOVR_EXCL_STOP
        return ec;
    #endif
}

int Tracer::trace_with_strace(const fs::path &strace_path) {
    auto temp_path = build_dir_path / fs::unique_path();

    // Run the build command with build tracer.
    auto orig_cwd = fs::current_path();
    int ec = 0;
    try {
        bp::child b(
            strace_path, "-f", "-v", "-s", "65535", "-e", "trace=execve", "-o", temp_path,
            bp::args(build_commands),
            bp::start_dir=build_dir_path,
            // Has to change PWD manually since we rely on it and start_dir doesn't change it.
            bp::env["PWD"]=build_dir_path.string());
        b.wait();
        ec = b.exit_code();
    //GCOVR_EXCL_START
    } catch (boost::process::process_error &err) {
        LOG(warning) << "Failed to run " << strace_path << ", reason:" << err.what();
        ec = 1;
    }
    if(ec) {
        fs::remove(temp_path);
        return ec;
    }
    // GCOVR_EXCL_STOP

    fs::ifstream strace_in{ temp_path };

    json cdb = json::array();

    // \d+\s+execve\("([^\"]+)", (\[[^\[]+\]), \[[^\[]*"PWD=([^\"]+)"[^\[]*\](\) = 0| <unfinished \.\.\.>)
    // group 1: binary, /bin/grep
    // group 2: arguments, ["grep", "project/mt7687_hdk/"]
    // group 3: pwd, /work/LinkIt_SDK_public-4.6.1
    boost::regex execve_line{"\\d+\\s+execve\\(\"([^\\\"]+)\", (\\[[^\\[]+\\]), \\[[^\\[]*\"PWD=([^\\\"]+)\"[^\\[]*\\](\\) = 0| <unfinished \\.\\.\\.>)"};
    boost::smatch match;
    for(std::string line; std::getline(strace_in, line);) {
        if(boost::regex_match(line, match, execve_line)) {
            std::string binary(match[1].first, match[1].second);

            // Filter by binary file name.
            if(!binaries.count(fs::path(binary).filename().string())) {
                continue;
            }

            json cmd = json::object();
            cmd["directory"] = std::string(match[3].first, match[3].second);
            try {
                cmd["arguments"] = json::parse(std::string(match[2].first, match[2].second));
            //GCOVR_EXCL_START
            } catch(...) {
                LOG(warning) << "NOT JSON" << std::string(match[2].first, match[2].second);
                continue;
            }
            //GCOVR_EXCL_STOP

            // Comment out since is not the behaviour of BEAR.
            // cmd["arguments"][0] = binary;

            cdb.push_back(cmd);
        }
    }
    strace_in.close();

    fs::ofstream cdb_out{ compile_db_path };
    cdb_out << cdb.dump(2);
    cdb_out.close();

    fs::remove(temp_path);
    return 0;
}

//GCOVR_EXCL_START
int Tracer::trace_with_windbg() {
    auto build_tracer_path = tool_root / "bin" / fs::path("win-tracer.exe");
    auto config_file = build_dir_path / fs::path("compiler-config.json");

    // Generate the config file.
    fs::ofstream out(config_file);
    json config = json::array();
    for(const auto &p: binaries) {
        config.push_back(p.second);
    }
    out << config.dump() << std::endl;
    out.close();

    int ec = 0;
    try {
        bp::child b(
            build_tracer_path,  "/o", compile_db_path,
            "/c", config_file,
            bp::args(build_commands),
            bp::start_dir = build_dir_path);
        b.wait();
        ec = b.exit_code();
    } catch (boost::process::process_error &err) {
        LOG(warning) << "Failed to run " << build_tracer_path << ", reason:" << err.what();
        ec = 1;
    }
    fs::remove(config_file);
    return ec;
}
//GCOVR_EXCL_STOP

boost::optional<ErrorCode> Tracer::trace() {
    // Initial time.
    Stopwatch watch;

    // Run the prebuild command.
    if(prebuild.length()) {
        try {
            //bp::child pb(prebuild, bp::start_dir=build_dir_path);
            //pb.wait();
            //bp::system(prebuild, bp::start_dir=build_dir_path);
            std::vector<std::string> args {"-c", prebuild};
            bp::system(bp::search_path("sh"), args, bp::start_dir=build_dir_path);
            //LOG(debug) << "Prebuild exited with code: " << pb.exit_code();
        //GCOVR_EXCL_START
        } catch (const std::exception& e) {
            LOG(warning) << "Unkown exception when running prebuild. " << e.what();
        }
        //GCOVR_EXCL_STOP
    }

    int ec = 0;
    switch(tracing_method) {
    case TracingMethod::BEAR:
        LOG(info) << "Tracing Method: Bear";
        ec = trace_with_bear();
        break;
    case TracingMethod::STRACE: {
        LOG(info) << "Tracing Method: STrace";
        auto strace_path = bp::search_path("strace");
        //GCOVR_EXCL_START
        if(!strace_path.string().length()) {
            LOG_EC(critical, EC_STRACE_NOT_FOUND) << "`strace` not found, please install it, or choose a different tracing method.";
            return EC_STRACE_NOT_FOUND;
        }
        //GCOVR_EXCL_STOP
        ec = trace_with_strace(strace_path);
        break;
    }
    //GCOVR_EXCL_START
    case TracingMethod::WINDBG:
        LOG(info) << "Tracing Method: WinDBG";
        ec = trace_with_windbg();
        break;
    }
    //GCOVR_EXCL_STOP

    // Log time
    LOG(debug) << "TRACE TIME: " << watch.get_duration();

    if(ec == 0) {
        LOG(debug) << "Build tracer exited successfully";
        return boost::none;
    }
    else {
        // Still return and continue even tracing failed.
        LOG(debug) << "Build tracer exited with compilation failure: " << ec;
        return EC_COMPILATION_FAILURE;
    }
}