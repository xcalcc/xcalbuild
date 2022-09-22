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

#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>

#include "fwd.hpp"

namespace xcal {

/**
 * @brief Probe a native binary according to tool profile.
 */
class Prober {
public:
    /**
     * @brief Construct a new Prober object
     */
    Prober() {}

    /**
     * @brief Probe a toolchain.
     *
     * @param cdb The compile database with calls to tools in the toolchain.
     * @param toolchain_profile The toolchain profile.
     */
    void probe_toolchain(const nlohmann::json &cdb, const ToolchainProfile &toolchain_profile);

    /**
     * @brief Generate macro test input
     *
     * @param path the input file path
     * @param macros the macro test spec
     */
    void generate_macros_test_file(
        const boost::filesystem::path &path,
        const nlohmann::json &macros);

    /**
     * @brief Parse macro test output and get actions
     * Returning the value since `json *` doesn't seem to be reliable.
     * @param path the output file path
     * @param macros the macro test spec
     * @return std::vector<const json *> the list of actions to be taken.
     */
    std::vector<nlohmann::json> parse_macro_expansions(
        const boost::filesystem::path &path,
        const nlohmann::json &macros);

    /**
     * @brief Probe macro definitions with the corresponding profile.
     *
     * @param binary_path the tool
     * @param tool_profile the tool profile
     * @param is_cxx probe C++ macros, otherwise C macros.
     */
    void probe_macros(boost::filesystem::path binary_path, ToolProfile *tool_profile, bool is_cxx);
};

}
