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

#include <cstring>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "command-line.hpp"
#include "error-code.hpp"
#include "config.h"

namespace po = boost::program_options;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

using namespace xcal;

boost::optional<ErrorCode> CommandLine::parse(int argc, char **argv) {
    std::string binary = argv[0];

    std::string profile;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("builddir,i",
                po::value<std::string>(&build_dir)->required(),
                "build directory")
            ("outputdir,o",
                po::value<std::string>(&output_dir)->required(),
                "output directory")
            ("prebuild,p",
                po::value<std::string>(&prebuild),
                "prebuild command, such as 'cmake .','./configure'... ")
            ("fkey,f",
                po::value<std::string>(&fkey),
                "dir filter keyword, use ; as seperator")
            ("lkey,l",
                po::value<std::string>(&lkey),
                "link filter keyword, use ; as seperator")
            ("fwl",
                po::value<std::string>(&wlfcmd),
                "whitelist files filter command")
            ("fbl",
                po::value<std::string>(&blfcmd),
                "blacklist files filter command")
            ("process_link_using_compiler",
                po::bool_switch(&process_link_using_compiler)->default_value(false),
                "process source code file whose target file is linked through use of compiler command, default is false. source code file whose target file is linked through use of linker command such as ld will be processed direclty by xcalbuild.")
            ("debug",
                po::bool_switch(&debug)->default_value(false),
                "debug info")
            ("local_log",
                po::bool_switch(&local_log)->default_value(false),
                "local log file mode, default is false when called by other entities")
            ("preprocessor",
                po::value<std::string>(&preprocessor)->default_value("default"),
                "preprocessor command")
            ("trace_id,t",
                po::value<std::string>(&trace_id)->default_value(""),
                "trace id for logging, not in use in this version")
            ("span_id,s",
                po::value<std::string>(&span_id)->default_value(""),
                "span id for logging")
            ("tracing_method,m",
                po::value<std::string>(&tracing_method)->default_value(
#ifdef _WIN32
                "windbg"
#else
                "dynamic"
#endif
                ),
                "the method for build tracing, one of 'dynamic', 'static' or 'windbg'")
            ("parallel,j",
                po::value<int>(&parallelism)->default_value(1),
                "the pre-processing parallelism, default is 1")
            ("profile",
                po::value<std::string>(&profile)->default_value(
#ifdef _WIN32
                "windows-auto"
#else
                "linux-auto"
#endif
                ),
                "the toolchain profile to be used, default is to auto-detect")
            ("version,v",
                "display version")
            ("help,h",
                "display help message");

        // Had to do this manually.
        bool start_build_commands = false;
        for (int i = 0; i < argc; i++) {
            if (start_build_commands) {
                build_commands.push_back(argv[i]);
            } else if (std::strcmp(argv[i], "--") == 0) {
                start_build_commands = true;
            }
        }

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage:\n";
            std::cout << "xcalbuild [options] -- build commands\n";
            std::cout << desc << "\n";
            return EC_NONE;
        } else if (vm.count("version")) {
            std::cout << "xcalbuild ("
                      << std::string{PROJECT_BUILD_ARCH} << "-"
                      << std::string{PROJECT_BUILD_PLATFORM} << ") "
                      << std::string{PROJECT_VERSION}
                      << " [" << std::string{PROJECT_SOURCE_VERSION}.substr(0,7) << "]\n";
            return EC_NONE;
        }
        po::notify(vm);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EC_INCORRECT_COMMAND_LINE;
    //GCOVR_EXCL_START
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;
        return EC_UNKOWN;
    }
    //GCOVR_EXCL_STOP

    tool_root = fs::system_complete(binary).parent_path().parent_path();

    // Validate input.
    if(!build_commands.size()) {
        std::cerr << "Empty build command." << std::endl;
        return EC_INCORRECT_COMMAND_LINE;
    }

    if(!fs::exists(build_dir)) {
        std::cerr << "Build directory '" << build_dir << "' does not exist." << std::endl;
        return EC_INCORRECT_COMMAND_LINE;
    }

    if(!fs::exists(output_dir)) {
        std::cerr << "Output directory '" << output_dir << "' does not exist." << std::endl;
        return EC_INCORRECT_COMMAND_LINE;
    }

    if(!(tracing_method == "static" || tracing_method == "dynamic" || tracing_method == "windbg")) {
        std::cerr << "Invalid tracing method '" << tracing_method << "'" << std::endl;
        return EC_INCORRECT_COMMAND_LINE;
    }

    toolchain_profile = tool_root / "profiles" / profile;
    if(!fs::exists(toolchain_profile)) {
        std::cerr << "toolchain profile '" << toolchain_profile << "' does not exist." << std::endl;
        return EC_INCORRECT_COMMAND_LINE;
    }

    if(profile != "linux-auto" && profile != "windows-auto") {
        auto_detect_toolchain_profile = false;
    }

    return boost::none;
}

boost::optional<ErrorCode> CommandLine::parse_config() {
    auto config_path = tool_root / "config";
    try {
        boost::property_tree::ini_parser::read_ini(config_path.string(), config);
    }  catch (std::exception &e) {
        std::cerr << "Failed to parse config file " << e.what() << std::endl;
        return EC_INCORRECT_CONFIG_FILE;
    //GCOVR_EXCL_START
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;
        return EC_UNKOWN;
    }
    //GCOVR_EXCL_STOP

    return boost::none;
}

const std::string &CommandLine::get_build_dir() const {
    return build_dir;
}

const std::string &CommandLine::get_output_dir() const {
    return output_dir;
}

const std::string &CommandLine::get_fkey() const {
    return fkey;
}

const std::string &CommandLine::get_lkey() const {
    return lkey;
}

const std::string &CommandLine::get_wlfcmd() const {
    return wlfcmd;
}

const std::string &CommandLine::get_blfcmd() const {
    return blfcmd;
}

const std::string &CommandLine::get_prebuild() const {
    return prebuild;
}

const std::string &CommandLine::get_preprocessor() const {
    return preprocessor;
}

const std::string &CommandLine::get_trace_id() const {
    return trace_id;
}

const std::string &CommandLine::get_span_id() const {
    return span_id;
}

TracingMethod CommandLine::get_tracing_method() const {
    if(tracing_method == "static") {
        return TracingMethod::STRACE;
    }
    //GCOVR_EXCL_START
    if (tracing_method == "windbg") {
        return TracingMethod::WINDBG;
    }
    //GCOVR_EXCL_STOP
    return TracingMethod::BEAR;
}

const std::vector<std::string> &CommandLine::get_build_commands() const {
    return build_commands;
}

const fs::path &CommandLine::get_toolchain_profile() const {
    return toolchain_profile;
}

bool CommandLine::is_auto_detect_toolchain_profile() const {
    return auto_detect_toolchain_profile;
}

const fs::path &CommandLine::get_tool_root() const {
    return tool_root;
}

int CommandLine::get_parallelism() const {
    return parallelism;
}

bool CommandLine::need_process_link_using_compiler() const {
    return process_link_using_compiler;
}

bool CommandLine::is_debug() const {
    return debug;
}

bool CommandLine::is_local_log() const {
    return local_log;
}

boost::optional<std::string> CommandLine::get_config(const char *key) const {
    auto res = config.get_optional<std::string>(key);
    // Treat empty string as null value as well.
    if(res && res->length() == 0) {
        res = boost::none;
    }
    return res;
}

std::string CommandLine::get_cdb_name() const {
    // TODO: this section might be named other than "linux"?
    return get_config("linux.CDB_NAME").get_value_or("compile_commands.json");
}

std::string CommandLine::get_source_list_file_name() const {
    return get_config("linux.SOURCE_FILES").get_value_or("source_files.json");
}

std::string CommandLine::get_preprocess_dir_name() const {
    return get_config("linux.PREPROCESS").get_value_or("preprocess");
}

boost::property_tree::ptree CommandLine::get_properties_template() const {
    if(auto tree_opt = config.get_child_optional("PROPERTY_KEY")) {
        return boost::property_tree::ptree(*tree_opt);
    }
    return boost::property_tree::ptree();
}
