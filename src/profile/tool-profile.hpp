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

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/regex.hpp>

#include <nlohmann/json.hpp>

#include "error-code.hpp"
#include "option.hpp"

namespace xcal {

/**
 * @brief A text substitution by string replacement.
 *
 */
struct StringSubstitution {
    std::string pattern;
    std::string replacement;

    StringSubstitution(std::string &&pattern, std::string &&replacement):
        pattern(pattern), replacement(replacement){}
};

/**
 * @brief A text substitution by regex replacement.
 *
 */
struct RegexSubstitution {
    boost::regex pattern;
    std::string replacement;

    RegexSubstitution(std::string &&pattern, std::string &&replacement):
        pattern(pattern), replacement(replacement){}
};

/**
 * @brief Tool profile loader and info provider.
 */
class ToolProfile {
public:
    /**
     * @brief Construct a new Tool Profile object.
     * Actual loading is done in load().
     */
    ToolProfile() {}

    /**
     * @brief Load the profile from path.
     *
     * @param profile_path the path to the profile directory.
     * @return boost::optional<ErrorCode> return none if success, otherwise the exit code.
     */
    boost::optional<ErrorCode> load(const boost::filesystem::path &profile_path);

    /**
     * @brief Load the profile from JSON.
     *
     * @param profile_json the profile in JSON.
     * @return boost::optional<ErrorCode> return none if success, otherwise the exit code.
     */
    boost::optional<ErrorCode> load(const nlohmann::json &profile_json);
    /**
     * @brief Load the actionable part of the profile from JSON.
     * This is the second stage loading after the actions taken by the Prober.
     */
    void load_actionable();

    /**
     * @brief Return the set of binaries aliases specified in the profile.
     *
     * @return const std::set<std::string> & the set of binary names.
     */
    const std::set<std::string> &get_default_aliases() const;

    /**
     * @brief Get the response file config object
     *
     * @return const nlohmann::json & the config
     */
    const nlohmann::json &get_response_file_config() const;

    /**
     * @brief Parsed json work item.
     *
     * @param work_item the input work item.
     * @param result the result.
     * @return * boost::option<int> return none if success, otherwise the exit code.
     */
    boost::optional<ErrorCode> parse_work_item(
        const nlohmann::json &work_item,
        ParsedWorkItem &result) const;

    /**
     * @brief Process the source code according to rules in the compiler profile.
     * This includes replacing/removing text etc.
     *
     * @param input the source code in a string.
     * @return std::string the processed source code.
     */
    std::string process_source_code(std::string &&input) const;

    /**
     * @brief Get the per-file preprocessing options.
     * This is done by combining the prepend/append options, includes,
     * and options from native command line.
     *
     * @param target The output target.
     * @param format The C or C++ variant.
     * @return std::vector<std::string> the options.
     */
    std::vector<std::string> get_preprocessing_options(
        const std::string &target,
        FileFormat format,
        const ParsedWorkItem &parsed
    ) const;

private:
    /**
     * @brief Apply the actions defined in actions to the tool profile.
     *
     * @param actions the specified actions
     * @return true on success.
     * @return false on failure.
     */
    bool apply_action(const nlohmann::json &action);

    /// The loaded tool profile json object.
    nlohmann::json profile_json;

    /// The path to the profile parent dir.
    boost::filesystem::path profile_dir;

    /// The binary aliases.
    std::set<std::string> aliases;

    /// The binary aliases that treat input as C by default.
    std::set<std::string> c_aliases;

    /// The binary aliases that treat input as C++ by default.
    std::set<std::string> cxx_aliases;

    /// Default command kind for this tool. The actual kind might depend on arguments.
    CommandKind default_command_kind = CommandKind::CK_COMPILE;

    /// The response file config to be used by the tracer.
    nlohmann::json response_file_config = nlohmann::json::array();

    /// The list of options supported by this tool.
    std::vector<std::unique_ptr<Option>> options;

    /// The response file option.
    const Option *response_file_option = nullptr;

    /// The the preprocess option.
    const Option *preprocess_option = nullptr;

    /// The output option.
    const Option *output_option = nullptr;

    /// The pre-include option.
    const Option *pre_include_option = nullptr;

    /// The system include path option.
    const Option *sys_inc_path_option = nullptr;

    /// The option prefix, if any.
    boost::optional<std::string> option_prefix = boost::none;

    /// Options to be appent to a C preprocessing command line.
    std::vector<std::string> c_prepend_preprocessing_options;

    /// Options to be appent to a C++ preprocessing command line.
    std::vector<std::string> cxx_prepend_preprocessing_options;

    /// Options to be appent to a C preprocessing command line.
    std::vector<std::string> c_append_preprocessing_options;

    /// Options to be appent to a C++ preprocessing command line.
    std::vector<std::string> cxx_append_preprocessing_options;

    /// Options to be prepent to C scan options.
    std::vector<std::string> c_prepend_scan_options;

    /// Options to be prepent to C++ scan options.
    std::vector<std::string> cxx_prepend_scan_options;

    /// C pre-include files, completed with the profile directory.
    std::vector<std::string> c_pre_includes;

    /// C++ pre-include files, completed with the profile directory.
    std::vector<std::string> cxx_pre_includes;

    /// C system include paths, completed with the profile directory.
    std::vector<std::string> c_sys_inc_paths;

    /// C++ system include paths, completed with the profile directory.
    std::vector<std::string> cxx_sys_inc_paths;

    /// Known source file extensions.
    std::map<std::string, FileFormat> source_extensions;

    /// Known target file extensions.
    std::map<std::string, FileFormat> target_extensions;

    /// The text substitutions.
    std::vector<boost::variant<StringSubstitution, RegexSubstitution>> text_substitutions;

    friend class Prober;
};

}
