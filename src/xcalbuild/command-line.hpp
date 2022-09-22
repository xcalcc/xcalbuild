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
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#include "fwd.hpp"
#include "error-code.hpp"

namespace xcal {

/**
 * @brief The command line processor and information prodiver.
 *
 */
class CommandLine {
public:
    /**
     * @brief Parse the command line.
     *
     * @param argc The command line args count.
     * @param argv The command line args.
     *
     * @return boost::optional<ErrorCode> return none if parsed correctly, otherwise return the exit code.
     */
    boost::optional<ErrorCode> parse(int argc, char **argv);
    /**
     * @brief Parse the config file.
     *
     * @return boost::optional<ErrorCode> return none if parsed correctly, otherwise return the exit code.
     */
    boost::optional<ErrorCode> parse_config();

    /**
     * @brief Get the build dir.
     *
     * @return const std::string&
     */
    const std::string &get_build_dir() const;

    /**
     * @brief Get the output dir.
     *
     * @return const std::string&
     */
    const std::string &get_output_dir() const;

    /**
     * @brief Get the prebuild command.
     *
     * @return const std::string&
     */
    const std::string &get_prebuild() const;

    /**
     * @brief Get the preprocessor specified.
     *
     * @return const std::string&
     */
    const std::string &get_preprocessor() const;

    /**
     * @brief Get the directory filter keyword
     *
     * @return const std::string&
     */
    const std::string &get_fkey() const;

    /**
     * @brief Get the linker filter keyword
     *
     * @return const std::string&
     */
    const std::string &get_lkey() const;

    /**
     * @brief Get cmd for whitelist files
     *
     * @return const std::string&
     */
    const std::string &get_wlfcmd() const;

    /**
     * @brief Get cmd for blacklist files
     *
     * @return const std::string&
     */
    const std::string &get_blfcmd() const;

    /**
     * @brief Get the trace id
     *
     * @return const std::string&
     */
    const std::string &get_trace_id() const;

    /**
     * @brief Get the span id
     *
     * @return const std::string&
     */
    const std::string &get_span_id() const;

    /**
     * @brief Get the tracing method specified.
     *
     * @return TracingMethod
     */
    TracingMethod get_tracing_method() const;

    /**
     * @brief Get the pre-processing parallelism specified.
     *
     * @return int
     */
    int get_parallelism() const;

    /**
     * @brief Get the native build commands
     *
     * @return const std::vector<std::string>&
     */
    const std::vector<std::string> &get_build_commands() const;

    /**
     * @brief Get the toolchain profile name.
     *
     * @return const boost::filesystem::path& the toolchain profile path.
     */
    const boost::filesystem::path &get_toolchain_profile() const;

    /**
     * @brief CHeck if we need to detect toolchain profile.
     *
     * @return true we need to detect toolchain profile.
     * @return false otherwise.
     */
    bool is_auto_detect_toolchain_profile() const;

    /**
     * @brief Get the path to the root directory of the tool.
     *
     * @return const boost::filesystem::path&
     */
    const boost::filesystem::path&get_tool_root() const;

    /**
     * @brief process source code file whose target file is linked through use of compiler command.
     * source code file whose target file is linked through use of linker command such as ld will be processed direclty by xcalbuild.
     *
     * @return true need process.
     * @return false(default) no need process.
     */
    bool need_process_link_using_compiler() const;

    /**
     * @brief Debug mode flag.
     *
     * @return true in debug mode.
     * @return false not in debug mode.
     */
    bool is_debug() const;

    /**
     * @brief Local log file mode flag
     *
     * @return false (default) when directing log to standard output
     * @return true when directing log to local file under output_dir
     */
    bool is_local_log() const;

    /**
     * @brief Get the compilation database name.
     *
     * @return std::string the file name.
     */
    std::string get_cdb_name() const;

    /**
     * @brief Get the source list file name.
     *
     * @return std::string the file name.
     */
    std::string get_source_list_file_name() const;

    /**
     * @brief Get the preprocess output dir name.
     *
     * @return std::string the dir name.
     */
    std::string get_preprocess_dir_name() const;

    /**
     * @brief Get the config value for the key
     *
     * @param key the key in section.key format. E.g. `linux.CDB_NAME`.
     * @return boost::optional<std::string> the value if any.
     */
    boost::optional<std::string> get_config(const char *key) const;

    /**
     * @brief Get a properties template.
     * This is used to create a xcalibyte.properties file.
     *
     * @return boost::property_tree::ptree an empty properties object.
     */
    boost::property_tree::ptree get_properties_template() const;

private:
    std::string build_dir = "";
    std::string output_dir = "";
    std::string prebuild = "";
    std::string preprocessor = "";
    std::string trace_id = "";
    std::string span_id = "";
    std::string tracing_method = "";
    std::string fkey = "";
    std::string lkey = "";
    std::string wlfcmd = "";
    std::string blfcmd = "";
    int parallelism = 1;
    std::vector<std::string> build_commands;
    boost::filesystem::path toolchain_profile;
    bool auto_detect_toolchain_profile = true;
    boost::filesystem::path tool_root;
    bool process_link_using_compiler = false;
    bool debug = false;
    bool local_log = false;
    boost::property_tree::ptree config;
};

}
