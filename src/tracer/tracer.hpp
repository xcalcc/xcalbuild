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

#pragma once

#include <string>
#include <vector>

#include <boost/optional.hpp>

#include "fwd.hpp"
#include "error-code.hpp"

namespace xcal {

/**
 * @brief Trace a native build.
 */
class Tracer {
public:

    /**
     * @brief Construct a new Tracer object
     *
     * @param tracing_method the tracing method
     * @param command_line the command line info
     * @param toolchain_profile the toolchain profile
     */
    Tracer (
        TracingMethod tracing_method,
        const CommandLine &command_line,
        const ToolchainProfile &toolchain_profile
    );

    /**
     * @brief Trace the build.
     */
    boost::optional<ErrorCode> trace();
private:
    int trace_with_bear();
    int trace_with_strace(const boost::filesystem::path &strace_path);
    int trace_with_windbg();

    /// The build tool probe info to guide tracing.
    TracingMethod tracing_method;

    /// The command line info.
    boost::filesystem::path build_dir_path;
    boost::filesystem::path compile_db_path;
    boost::filesystem::path tool_root;
    const std::string &prebuild;
    const std::vector<std::string> &build_commands;

    /// The toolchain binaries to trace.
    const std::map<std::string, nlohmann::json> binaries;
};

}
