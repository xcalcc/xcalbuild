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
#include <boost/regex.hpp>
#include <boost/assert.hpp>
#include <boost/algorithm/string.hpp>

#include <nlohmann/json.hpp>

#include "validation.hpp"
#include "toolchain-profile.hpp"
#include "error-code.hpp"
#include "logger.hpp"

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;
using json = nlohmann::json;

using namespace xcal;

boost::optional<ErrorCode> ToolchainProfile::load(const fs::path &profile_path) {
    // Load the toolchain profile json specified.
    LOG(info) << "Toolchain profile path: " << profile_path.string().c_str();
    fs::ifstream ifs{profile_path / "profile.json"};
    try {
        profile_json = json::parse(ifs, /*cb*/nullptr, /*allow_exceptions*/true, /*ignore_comments*/true);
    } catch (std::exception &e) {
        ifs.close();
        LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Failed to parse profile: " << e.what();
        return EC_INCORRECT_TOOLCHAIN_PROFILE;
    }
    ifs.close();

    return load(profile_json, profile_path);
}

boost::optional<ErrorCode> ToolchainProfile::load(const json &profile_json, const fs::path &profile_path) try {
    // Load the toolchain profile json specified.
    LOG(info) << "JSON profile size: " << profile_json.size();

    validate_json(profile_json, "tools", JSONType::JT_ARRAY, /*lookup_key*/true, /*print_json*/false);
    LOG(info) << "JSON profile tools size: " << profile_json["tools"].size();


    for(const auto &tool: profile_json["tools"]) {
        validate_json(tool, "tools", JSONType::JT_OBJECT_ARRAY, /*lookup_key*/false);
        validate_json(tool, "profile", JSONType::JT_STRING);
        // Load the tool profile.
        fs::path tool_path = tool["profile"].get<std::string>();
        if(!tool_path.is_absolute()) {
            tool_path = fs::complete(tool_path, profile_path);
        }

        auto p = std::make_unique<ToolProfile>();

        if(auto ec_opt = p->load(tool_path)) {
            return ec_opt;
        }

        // Add binary -> profile association.
        if(tool.count("aliases")) {
            validate_json(tool, "aliases", JSONType::JT_ARRAY);
            // The toolchain level aliases overrides the default ones.
            for(const auto &alias: tool["aliases"]) {
                validate_json(alias, "tools", JSONType::JT_STRING_ARRAY, /*lookup_key*/false);
                binaries[alias.get<std::string>()] = p.get();
            }
        } else {
            for(const auto &alias: p->get_default_aliases()) {
                binaries[alias] = p.get();
            }
        }
        // Add profile origins if any.
        if(tool.count("origin")) {
            validate_json(tool, "origin", JSONType::JT_ARRAY);
            profile_origins[p.get()] = std::vector<std::string>();
            for(const auto &origin: tool["origin"]) {
                validate_json(origin, "tools", JSONType::JT_STRING_ARRAY, /*lookup_key*/false);
                profile_origins[p.get()].push_back(origin.get<std::string>());
            }
        }

        profiles.push_back(std::move(p));
    }
    LOG(info) << "Loaded " << profiles.size() << " tool profiles";

    return boost::none;
} catch (IncorrectProfileException &e) {
    LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << e.what();
    return EC_INCORRECT_TOOLCHAIN_PROFILE;
}

void ToolchainProfile::load_actionable() {
    for(const auto &profile: profiles) {
        profile->load_actionable();
    }
}

ToolProfile *ToolchainProfile::get_mutable_tool_profile_and_binary_path(
    const nlohmann::json &work_item,
    fs::path &binary_path
) const {
    if(!work_item.contains("arguments")) {
        return nullptr;
    }

    const auto &arguments = work_item["arguments"];
    if(arguments.size() == 0) {
        return nullptr;
    }

    binary_path = fs::path(arguments[0].get<std::string>());

    // Removes the exe from windows binaries
    std::string binary = fs::change_extension(binary_path.filename(), "").string();
#if _WIN32
    // Windows is case sensitive, so make binary name lower case
    boost::algorithm::to_lower(binary);
#endif

    const auto &it = binaries.find(binary);

    if(it != binaries.end()) {
        return it->second;
    }
    else {
        return nullptr;
    }
}

const std::vector<std::string> *ToolchainProfile::get_profile_origins(const ToolProfile *profile) const {
    auto it = profile_origins.find((ToolProfile *)profile);
    if(it != profile_origins.end()) {
        return &(it->second);
    }
    return nullptr;
}

const ToolProfile *ToolchainProfile::get_tool_profile(const json &work_item) const {
    fs::path binary_path;
    return get_mutable_tool_profile_and_binary_path(work_item, binary_path);
}

std::map<std::string, nlohmann::json> ToolchainProfile::get_binaries_to_trace() const {
    std::map<std::string, nlohmann::json> res;
    for(const auto &p: binaries) {
        // {\"binary\": \"iccarm\", \"responseFileArgs\" : [{\"argument\": \"-f\", \"argFormat\" : [\"space\"]}]}
        json config;
        config["binary"] = p.first;
        config["responseFileArgs"] = p.second->get_response_file_config();
        res.insert(std::make_pair(p.first, std::move(config)));
    }
    return res;
}
