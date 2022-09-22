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

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/regex.hpp>

#include <nlohmann/json.hpp>

#include "tool-profile.hpp"
#include "fwd.hpp"

namespace xcal {

/**
 * @brief Toolchain profile loader and info provider.
 */
class ToolchainProfile {
public:
    /**
     * @brief Construct a new Toolchain Profile object.
     * Actual loading is done in load().
     */
    ToolchainProfile() {}

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
     * @param profile_path the path to the profile directory.
     * @return boost::optional<ErrorCode> return none if success, otherwise the exit code.
     */
    boost::optional<ErrorCode> load(const nlohmann::json &profile_json, const boost::filesystem::path &profile_path);

    /**
     * @brief Load the actionable parts of tool profiles from JSON.
     */
    void load_actionable();

    /**
     * @brief Return the set of compiler/linker binaries specified in the profile.
     * Also return the tracer config JSON objects for each of the binary.
     * @return std::map<std::string, nlohmann::json> the set of binaries and configs.
     */
    std::map<std::string, nlohmann::json> get_binaries_to_trace() const;

    /**
     * @brief Get the tool profile object for the binary specified in work_item.
     *
     * @param work_item a command line
     * @return const ToolProfile * the cooresponding profile, if any.
     */
    const ToolProfile *get_tool_profile(const nlohmann::json &work_item) const;

    /**
     * @brief Get the mutable tool profile and fill the binary path
     *
     * @param work_item a command line
     * @param binary_path the path to the binary
     * @return ToolProfile* the cooresponding profile, if any.
     */
    ToolProfile *get_mutable_tool_profile_and_binary_path(
        const nlohmann::json &work_item,
        boost::filesystem::path &binary_path) const;

    /**
     * @brief Get the origins of a profile if any.
     *
     * @param profile the profile to lookup.
     * @return const std:vector<std::string> * a pointer to the list of origins, nullptr if no such list.
     */
    const std::vector<std::string> *get_profile_origins(const ToolProfile *profile) const;

private:
    /// The loaded compiler profile json object.
    nlohmann::json profile_json;

    /// The referenced tool profiles.
    std::vector<std::unique_ptr<ToolProfile>> profiles;

    /// The interesting binaries.
    std::map<std::string, ToolProfile *> binaries;

    /// The origin of the profiles.
    std::map<ToolProfile *, std::vector<std::string>> profile_origins;
};

}
