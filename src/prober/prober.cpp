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

#include <unordered_set>

#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

#include "prober.hpp"
#include "toolchain-profile.hpp"
#include "tool-profile.hpp"
#include "path.hpp"
#include "logger.hpp"

namespace bp = boost::process;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;
using json = nlohmann::json;

using namespace xcal;

constexpr const char *xcal_prefix = "__XCAL__";
constexpr int xcal_prefix_length = 8;

void Prober::generate_macros_test_file(const fs::path &path, const json &macros) {
    fs::ofstream ofs{ path };
    for(const auto &item: macros.items()) {
        auto macro_text = item.key();
        // __XCAL____STDC_VERSION__ __STDC_VERSION__
        ofs << xcal_prefix << macro_text << " " << macro_text << std::endl;
    }
    ofs.close();
}

std::vector<json> Prober::parse_macro_expansions(const fs::path &path, const json &macros) {
    std::vector<json> res;
    fs::ifstream ifs{ path };
    for(std::string line; std::getline(ifs, line);){
        // __XCAL____STDC_VERSION__ __STDC_VERSION__
        if(ba::starts_with(line, xcal_prefix)) {
            auto rest = line.substr(xcal_prefix_length);
            for(const auto &macro: macros.items()) {
                auto key = macro.key();
                if(ba::starts_with(rest, key)) {
                    auto val = rest.substr(key.length() + 1);
                    LOG(info) << "Macro '" << key << "' expands to '" << val << "'";
                    auto values = macro.value();
                    if(values.count(val)) {
                        LOG(info) << "Apply actions for macro '" << key << "'";
                        res.push_back(values[val]);
                    }
                }
            }
        }
    }
    ifs.close();
    return res;
}

void Prober::probe_macros(fs::path binary_path, ToolProfile *tool_profile, bool is_cxx) {
    auto basename = fs::basename(binary_path);
    if((is_cxx && tool_profile->c_aliases.count(basename)) ||
       (!is_cxx && tool_profile->cxx_aliases.count(basename))) {
        // If checking C/C++ and binary is a C++/C alias, change to use a C/C++ alias if available.
        const auto &filter = is_cxx ? tool_profile->c_aliases : tool_profile->cxx_aliases;
        const auto &needed = is_cxx ? tool_profile->cxx_aliases : tool_profile->c_aliases;
        bool has_needed = needed.size() != 0;
        for(const auto &alias: has_needed ? needed : tool_profile->aliases) {
            // Only generic names, find one that's not in filter.
            if(!has_needed && filter.count(alias)) {
                continue;
            }
            auto alias_path =
                fs::change_extension(
                    binary_path.parent_path() / alias, fs::extension(binary_path));
            if(fs::exists(alias_path)) {
                binary_path = alias_path;
                break;
            }
        }
    }

    auto input_path = fs::change_extension(get_temp_path(), is_cxx ? ".cc" : ".c");
    auto output_path = get_temp_path();

    auto macro_property = is_cxx ? "probeCxxMacros" : "probeCMacros";
    auto macros = tool_profile->profile_json[macro_property];

    generate_macros_test_file(input_path, macros);

    LOG(info) << "Probing " << (is_cxx ? "C++" : "C") << " compiler '" << binary_path << "'";
    int ec = 0;
    try {
        bp::child pp(
            binary_path,
            bp::args(tool_profile->output_option->make_string_option(output_path.string())),
            bp::args(
                tool_profile->preprocess_option
                ? tool_profile->preprocess_option->make_string_option("")
                : std::vector<std::string>()
            ),
            input_path);

        pp.wait(); //wait for the process to exit
        ec = pp.exit_code();
    // GCOVR_EXCL_START
    } catch (boost::process::process_error &err) {
        LOG(warning) << "Failed to run " << binary_path << ", reason:" << err.what();
        ec = 1;
    }
    if(ec != 0) {
        LOG(warning) << "Failed to run " << binary_path << ", exit code:" << ec;
        fs::remove_all(input_path);
        fs::remove_all(output_path);
        return;
    }
    // GCOVR_EXCL_STOP

    // Parse output and apply actions.
    for(const json &actions: parse_macro_expansions(output_path, macros)) {
        for(const json &action: actions) {
            tool_profile->apply_action(action);
        }
    }

    fs::remove_all(input_path);
    fs::remove_all(output_path);
}

void Prober::probe_toolchain(const json &cdb, const ToolchainProfile &toolchain_profile) {
    std::unordered_set<ToolProfile *> probed;

    LOG(info) << "Compile Database Size: " << cdb.size();

    for(const json &work_item: cdb) {
        // Skip if no directory
        if(!work_item.count("directory")) {
            LOG(info) << "No directory, skipped";
            continue;
        }
        // Skip if no arguments
        if(!work_item.count("arguments")) {
            LOG(info) << "No arguments, skipped";
            continue;
        }
        // Skip if empty arguments
        if(!work_item["arguments"].size()) {
            LOG(info) << "Empty arguments, skipped";
            continue;
        }

        fs::path binary_path;
        auto profile =
            toolchain_profile.get_mutable_tool_profile_and_binary_path(
                work_item,
                binary_path);
        // Filter unknown or handled profiles.
        if(!profile) {
            LOG(info) << "Unknown profile, skipped";
            continue;
        }
        if(!probed.insert(profile).second) {
            //LOG(info) << "Unhandled profile, skipped";
            continue;
        }

        if(profile->output_option) {
            auto binary_str = binary_path.string();
            auto dir_str = work_item["directory"].get<std::string>();
            auto profile_path = get_full_path_str(binary_str, dir_str, /*search*/true);

            // Probe C++ Macros
            if(profile->profile_json.count("probeCxxMacros")) {
                // This is not empty after validation
                probe_macros(profile_path, profile, /*is_cxx*/true);
            }
            // Probe C Macros
            if(profile->profile_json.count("probeCMacros")) {
                // This is not empty after validation
                probe_macros(profile_path, profile, /*is_cxx*/false);
            }
        }
    }
}