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

#include <set>
#include <string>
#include <vector>
#include <mutex>

#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

#include "fwd.hpp"
#include "error-code.hpp"

namespace xcal {
/**
 * @brief A result file.
 *
 */
struct ResultFile {
    /// The path to the file.
    boost::filesystem::path path;

    /// File checksum.
    std::string checksum;

    ResultFile(
        boost::filesystem::path &&path,
        std::string &&checksum):
        path(path), checksum(checksum) {}
};

/**
 * @brief toolchain processing outcome.
 *
 */
struct CCResult {
    /// Preprocessing output.
    ResultFile file;

    /// The target specified on the command line, a .o file.
    std::string target;

    /// The source file full path.
    std::string source;

    /// The source file format.
    FileFormat format;

    /// The pp file name.
    std::string pp_file_name;

    /// C scan options to be passed to backend.
    std::vector<std::string> c_scan_options;

    /// C++ scan options to be passed to backend.
    std::vector<std::string> cxx_scan_options;

    /// Non-system dependencies mentioned in the preprocessor output.
    std::set<std::string> deps;

    CCResult(
        ResultFile &&file,
        std::string &&target,
        std::string &&source,
        FileFormat format,
        std::string &&pp_file_name,
        std::vector<std::string> &&c_scan_options,
        std::vector<std::string> &&cxx_scan_options,
        std::set<std::string> &&deps):
        file(file),
        target(target),
        source(source),
        format(format),
        pp_file_name(pp_file_name),
        c_scan_options(c_scan_options),
        cxx_scan_options(cxx_scan_options),
        deps(deps) {}
};

/**
 * @brief Assember processing outcome.
 *
 */
struct ASResult {
    /// The target specified on the command line, .o file.
    std::string target;

    /// The sources specified on the command line, .s file.
    std::string source;

    ASResult(std::string &&target, std::string &&source):
        target(target), source(source) {}
};

/**
 * @brief Linker processing outcome.
 *
 */
struct LDResult {
    /// The target specified on the command line, library or executable.
    std::string target;

    /// The sources specified on the command line, .o files.
    std::vector<std::string> sources;

    LDResult(
        std::string &&target,
        std::vector<std::string> &&sources):
        target(target), sources(sources) {}
};

/**
 * @brief Processing a build spec and produce output for scanner.
 *
 */
class BuildProcessor {
public:
    /**
     * @brief Construct a new Build Processor object
     *
     * @param toolchain_profile The toolchain profile to be used.
     * @param output_dir The output path.
     * @param parallelism The number of work-item-processors to be used.
     */
    BuildProcessor(
        ToolchainProfile &toolchain_profile,
        const CommandLine &command_line,
        int parallelism
    );

    /**
     * @brief Process the build spec.
     *
     * @return boost::optional<ErrorCode> return none if success, otherwise the exit code.
     */
    boost::optional<ErrorCode> process();

    /**
     * @brief Get the directory filter vector.
     *
     * @return const std::vector<std::string>&
     */
    const std::vector<std::string> &get_filter_key() const;

    /**
     * @brief Get the linker filter vector.
     *
     * @return const std::vector<std::string>&
     */
    const std::vector<std::string> &get_linker_key() const;

    /**
     * @brief Get the whitelist files vector.
     *
     * @return const std::vector<std::string>&
     */
    const std::vector<std::string> &get_whitelist_files() const;

    /**
     * @brief Get the blacklist files vector.
     *
     * @return const std::vector<std::string>&
     */
    const std::vector<std::string> &get_blacklist_files() const;

    /**
     * @brief Append a compile result to the cc_results buffer.
     * This function is called by multiple writers (WorkItemProcessor).
     *
     * @param result the result to be appended.
     */
    void append_cc_result(CCResult &&result);

    /**
     * @brief Append an assembler result to the as_results buffer.
     * This function is called by multiple writers (WorkItemProcessor).
     *
     * @param result the result to be appended.
     */
    void append_as_result(ASResult &&result);

    /**
     * @brief Append a link result to the ld_results buffer.
     * This function is called by multiple writers (WorkItemProcessor).
     *
     * @param result the result to be appended.
     */
    void append_ld_result(LDResult &&result);

    /**
     * @brief Detect toolchain profile if set to auto-detect.
     *
     * @return std::string the new profile name.
     */
    std::string detect_toolchain_profile();

private:
    /**
     * @brief Load the compile database from path.
     *
     * @param path the compile database path.
     * @return boost::optional<ErrorCode> return none if success, otherwise the exit code.
     */
    boost::optional<ErrorCode> load_cdb(const boost::filesystem::path &path);

    /**
     * @brief Generate output directory and files.
     *
     * @return boost::optional<ErrorCode> return none if success, otherwise the exit code.
     */
    boost::optional<ErrorCode> generate_output();

    /// The toolchain profile to be used.
    ToolchainProfile &toolchain_profile;
    /// The command line info.
    const CommandLine &command_line;
    /// The output path etc.
    boost::filesystem::path output_path;
    /// The number of work-item-processors to be used.
    int parallelism;

    /// The loaded compile database;
    nlohmann::json cdb;

    /*
        These queues are wrote by the WorkItemProcessor,
        std::vector is thread safe, but only for single writer.
        WorkItemProcessor will only write the queue, and within
        the guard of the mutex.

        Using lockfree queue is also an option, but wait until
        we have a performance problem on concurrent writes,
        which is unlikely.
    */

    /// The TU processing results.
    std::vector<CCResult> cc_results;
    /// The mutex protecting tu_results.
    std::mutex cc_results_mutex;

    /// The link processing results.
    std::vector<ASResult> as_results;
    /// The mutex protecting link_results.
    std::mutex as_results_mutex;

    /// The link processing results.
    std::vector<LDResult> ld_results;
    /// The mutex protecting link_results.
    std::mutex ld_results_mutex;

    // The filter keyword vector.
    std::vector<std::string> filter_key;

    // The linker keyword vector.
    std::vector<std::string> linker_key;

    // The whitelist files vector
    std::vector<std::string> whitelist_files;

    // The blacklist files vector
    std::vector<std::string> blacklist_files;
};

}