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
#include <boost/variant.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/assert.hpp>

#include <nlohmann/json.hpp>

#include <iostream>
#if _WIN32
#include <sstream>
#endif

#include "validation.hpp"
#include "tool-profile.hpp"
#include "logger.hpp"
#include "path.hpp"

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;
using json = nlohmann::json;

using namespace xcal;

static const std::set<std::string> actionable_configs = {
    "cPrependPreprocessingOptions",
    "cAppendPreprocessingOptions",
    "cxxPrependPreprocessingOptions",
    "cxxAppendPreprocessingOptions",
    "cPrependScanOptions",
    "cxxPrependScanOptions",
    "cSystemIncludePaths",
    "cPreIncludes",
    "cxxSystemIncludePaths",
    "cxxPreIncludes",
};

static const std::vector<std::string> probes = {
    "probeCMacros",
    "probeCxxMacros",
};

boost::optional<ErrorCode> ToolProfile::load(const fs::path &profile_path) {
    // Load the tool profile json specified.
    LOG(info) << "Tool profile path: " << profile_path.string().c_str();

    fs::ifstream ifs{profile_path};
    try {
        profile_json = json::parse(ifs, /*cb*/nullptr, /*allow_exceptions*/true, /*ignore_comments*/true);
    } catch (std::exception &e) {
        ifs.close();
        LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Failed to parse profile: " << e.what();
        return EC_INCORRECT_TOOLCHAIN_PROFILE;
    }
    ifs.close();
    profile_dir = profile_path.parent_path();

    return load(profile_json);
}

boost::optional<ErrorCode> ToolProfile::load(const json &profile_json) try {
    // Parse binaries.
    LOG(info) << "JSON size: " << profile_json.size();
    const auto fill_string_set = [&](std::string prop, std::set<std::string> &s) {
        if(profile_json.count(prop)) {
            validate_json(profile_json, prop, JSONType::JT_ARRAY, /*lookup_key*/true, /*print_json*/false);
            for(const auto &elem: profile_json[prop]) {
                validate_json(elem, prop, JSONType::JT_STRING_ARRAY, /*lookup_key*/false);
                s.insert(elem.get<std::string>());
            }
        }
    };
    fill_string_set("aliases", aliases);
    fill_string_set("cAliases", c_aliases);
    fill_string_set("cxxAliases", cxx_aliases);

    // default command kind.
    validate_json(profile_json, "defaultCommandKind", JSONType::JT_STRING, /*lookup_key*/true, /*print_json*/false);
    auto kind_opt = command_kind_from_json(profile_json["defaultCommandKind"]);
    validate_optional(kind_opt, "defaultCommandKind");
    default_command_kind = *kind_opt;
    LOG(info) << "Default command kind: " << command_kind_to_string(default_command_kind);

    // Option prefix.
    if(profile_json.count("optionPrefix")) {
        const auto &j = profile_json["optionPrefix"];
        validate_json(j, "optionPrefix", JSONType::JT_STRING, /*lookup_key*/false);
        option_prefix = j.get<std::string>();
    }

    // Parse options
    validate_json(profile_json, "options", JSONType::JT_ARRAY, /*lookup_key*/true, /*print_json*/false);
    const char *defaultPrefix = option_prefix ? option_prefix->c_str() : nullptr;
    for(const auto &option: profile_json["options"]) {
        Option *op = parse_option(option, defaultPrefix);
        if(!op) {
            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Unable to parse " << option.dump();
            return EC_INCORRECT_TOOLCHAIN_PROFILE;
        }
        options.emplace_back(op);

        // Handle special options, the last one in the list wins.
        if(op->type == OptionType::OT_RESPFILE) {
            // Response file options.
            // [{\"argument\": \"-f\", \"argFormat\" : [\"space\"]}]
            for(const auto &alias: option["aliases"]) {
                json op;
                op["argument"] = alias;
                op["argFormat"] = option["argFormat"];
                response_file_config.push_back(std::move(op));
            }
            response_file_option = op;
        } else if(op->type == OptionType::OT_PREPROCESS) {
            preprocess_option = op;
        } else if(op->type == OptionType::OT_OUTPUT) {
            output_option = op;
        } else if(op->type == OptionType::OT_PRE_INCLUDE) {
            pre_include_option = op;
        } else if(op->type == OptionType::OT_SYS_INC_PATH) {
            sys_inc_path_option = op;
        }
    }

    // Load file extensions.
    const auto load_ext = [&](std::string key, std::map<std::string, FileFormat> &m) {
        if(profile_json.count(key)) {
            validate_json(profile_json, key, JSONType::JT_OBJECT, /*lookup_key*/true, /*print_json*/false);
            for(const auto &o: profile_json[key].items()) {
                auto ty = file_format_from_string(o.key());
                validate_optional(ty, o.key(), profile_json[key]);
                validate_json(o.value(), o.key(), JSONType::JT_ARRAY, /*lookup_key*/false);
                for(const auto &ext: o.value()) {
                    validate_json(ext, o.key(), JSONType::JT_STRING_ARRAY, /*lookup_key*/false, /*print_json*/true, /*allow_empty*/true);
                    m.insert(std::make_pair(ext.get<std::string>(), *ty));
                }
            }
        }
    };
    load_ext("sourceExtensions", source_extensions);
    load_ext("targetExtensions", target_extensions);

    // Load text substitutions.
    if(profile_json.count("textSubstitutions")) {
        validate_json(profile_json, "textSubstitutions",
            JSONType::JT_ARRAY, /*lookup_key*/true, /*print_json*/false);
        for(const auto &json: profile_json["textSubstitutions"]) {
            validate_json(json, "textSubstitutions", JSONType::JT_OBJECT_ARRAY, /*lookup_key*/false);
            validate_json(json, "replacement", JSONType::JT_STRING,
                /*lookup_key*/true, /*print_json*/true, /*allow_empty*/true);

            if(json.count("regex")) {
                validate_json(json, "regex", JSONType::JT_STRING);
                RegexSubstitution sub(
                    json["regex"].get<std::string>(),
                    json["replacement"].get<std::string>());
                text_substitutions.push_back(std::move(sub));
            } else if(json.count("string")) {
                validate_json(json, "string", JSONType::JT_STRING);
                StringSubstitution sub(
                    json["string"].get<std::string>(),
                    json["replacement"].get<std::string>());
                text_substitutions.push_back(std::move(sub));
            } else {
                LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "In " << json.dump() << ", neither property 'string' nor 'regex' exists.";
                return EC_INCORRECT_TOOLCHAIN_PROFILE;
            }
        }
    }

    // Validate actionable items.
    for(const auto &p: actionable_configs) {
        if(profile_json.count(p)) {
            validate_json(profile_json, p, JSONType::JT_ARRAY,
                /*lookup_key*/true, /*print_json*/false, /*allow_empty*/true);
            for(const auto &j: profile_json[p]) {
                validate_json(j, p, JSONType::JT_STRING_ARRAY, /*lookup_key*/false);
            }
        }
    }

    // Validate probe directives.
    for(const auto &p: probes) {
        // p = probeCMacros
        if(profile_json.count(p)) {
            validate_json(profile_json, p, JSONType::JT_OBJECT, /*lookup_key*/true, /*print_json*/false);
            for(const auto &m: profile_json[p].items()) {
                // m = "__STDC_VERSION__": {...}
                validate_json(m.value(), m.key(), JSONType::JT_OBJECT, /*lookup_key*/false);
                for(const auto &v: m.value().items()) {
                    // v = "199409L": [{...}...]
                    validate_json(v.value(), v.key(), JSONType::JT_ARRAY, /*lookup_key*/false);
                    for(const auto &a: v.value()) {
                        // a = {"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}
                        validate_json(a, v.key(), JSONType::JT_OBJECT_ARRAY, /*lookup_key*/false);
                        validate_json(a, "config", JSONType::JT_STRING);
                        auto config = a["config"].get<std::string>();
                        if(!actionable_configs.count(config) || !profile_json.count(config)) {
                            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Incorrection config to act on \n" << a.dump(2);
                            return EC_INCORRECT_TOOLCHAIN_PROFILE;
                        }
                        validate_json(a, "action", JSONType::JT_STRING);
                        auto act = a["action"].get<std::string>();
                        if(act != "prepend") {
                            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Incorrection action type '" << act << "'\n" << a.dump(2);
                            return EC_INCORRECT_TOOLCHAIN_PROFILE;
                        }
                        validate_json(a, "value", JSONType::JT_ARRAY);
                        for(const auto &av: a["value"]) {
                            // av = "-std=gnu89"
                            validate_json(av, "value", JSONType::JT_STRING_ARRAY, /*lookup_key*/false);
                        }
                    }
                }
            }
        }
    }

    return boost::none;
} catch (IncorrectProfileException &e) {
    LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << e.what();
    return EC_INCORRECT_TOOLCHAIN_PROFILE;
}

bool ToolProfile::apply_action(const nlohmann::json &action) {
    auto config = action["config"].get<std::string>();
    auto act = action["action"].get<std::string>();
    // The only supported type for now.
    if(act == "prepend") {
        json &orig = profile_json[config];
        json val = action["value"];
        orig.insert(orig.begin(), val.begin(), val.end());
    }
    return true;
}

void ToolProfile::load_actionable() {
    LOG(info) << "Actionable parts: " << profile_dir;
    // Additional options.
    const auto fill_string_vec = [&](std::string prop, std::vector<std::string> &vec) {
        if(profile_json.count(prop)) {
            vec.reserve(profile_json[prop].size());
            for(const auto &elem: profile_json[prop]) {
                vec.push_back(elem.get<std::string>());
            }
        }
    };
    fill_string_vec("cPrependPreprocessingOptions", c_prepend_preprocessing_options);
    fill_string_vec("cxxPrependPreprocessingOptions", cxx_prepend_preprocessing_options);
    fill_string_vec("cAppendPreprocessingOptions", c_append_preprocessing_options);
    fill_string_vec("cxxAppendPreprocessingOptions", cxx_append_preprocessing_options);
    fill_string_vec("cPrependScanOptions", c_prepend_scan_options);
    fill_string_vec("cxxPrependScanOptions", cxx_prepend_scan_options);

    // Fill the pre-include files.
    const auto fill_path_vec = [&](std::string prop, std::vector<std::string> &vec) {
        if(profile_json.count(prop)) {
            vec.reserve(profile_json[prop].size());
            for(const auto &elem: profile_json[prop]) {
                vec.push_back(fs::complete(elem.get<std::string>(), profile_dir).string());
            }
        }
    };
    fill_path_vec("cPreIncludes", c_pre_includes);
    fill_path_vec("cSystemIncludePaths", c_sys_inc_paths);
    fill_path_vec("cxxPreIncludes", cxx_pre_includes);
    fill_path_vec("cxxSystemIncludePaths", cxx_sys_inc_paths);
}

const std::set<std::string> &ToolProfile::get_default_aliases() const {
    return aliases;
}

const nlohmann::json &ToolProfile::get_response_file_config() const {
    return response_file_config;
}

struct TextSubstitutionVisitor : public boost::static_visitor<void> {
    TextSubstitutionVisitor(std::string &&input): res(input) {}

    void operator()(const StringSubstitution &sub) {
        res = ba::replace_all_copy(res, sub.pattern, sub.replacement);
    }

    void operator()(const RegexSubstitution &sub) {
        res = boost::regex_replace(res, sub.pattern, sub.replacement);
    }

    std::string res;
};

std::string ToolProfile::process_source_code(std::string &&input) const {
    TextSubstitutionVisitor visitor(std::move(input));
    for(const auto &sub: text_substitutions) {
        boost::apply_visitor(visitor)(sub);
    }
    return visitor.res;
}


// Sometimes clang etc generates resp files or sub command lines
// with naively quoted options. Remove them before processing.
// e.g.
static std::string unquote(std::string &&orig) {
    auto sz = orig.size();
    if(orig[0] == '"' && orig[sz-1] == '"') {
        return orig.substr(1, sz - 2);
    }
    return orig;
}

boost::optional<ErrorCode> ToolProfile::parse_work_item(
    const json &work_item,
    ParsedWorkItem &parsed
) const {
    parsed.kind = default_command_kind;
    parsed.dir = work_item["directory"].get<std::string>();
    auto dir_path = fs::path(parsed.dir);

    // Built string vector representation of the options.
    std::vector<std::string> raw_args;
    for(const auto &arg_json: work_item["arguments"]) {
        raw_args.push_back(unquote(arg_json.get<std::string>()));
    }

    // Unfold the response file.
    // TODO: also likely need to fold back the response file when calling the native toolchain.
    if(response_file_option) {
        std::vector<std::string> args;
        auto it = raw_args.begin();
        std::string resp_arg;
        while(it != raw_args.end()) {
            if(response_file_option->match_and_get_arg(it, resp_arg)) {
                std::string resp_content;
                if(work_item.count("respfile")) {
                    resp_content = work_item["respfile"].get<std::string>();
                } else {
                    // Try to load the file.
                    auto path = fs::complete(resp_arg, parsed.dir);
                    if(fs::exists(path) && fs::is_regular_file(path)) {
                        fs::ifstream ifs{path};
                        std::ostringstream resp_oss;
                        resp_oss << ifs.rdbuf();
                        ifs.close();
                        resp_content = resp_oss.str();
                        LOG(info) << "Loaded options from response file " << path;
                    } else {
                        LOG(warning) << "Response file " << path
                                   << " is no longer available. Consider making it persist.";
                    }
                }
                // Insert the response file args.
                std::vector<std::string> resp_args;
                boost::split(resp_args, resp_content, boost::is_space());
                for(auto arg: resp_args) {
                    args.push_back(unquote(std::move(arg)));
                }
                break;
            } else {
                args.push_back(std::move(*it));
                ++it;
            }
        }
        // Move the rest, if any.
        if(it != raw_args.end()) {
            args.insert(args.end(), std::make_move_iterator(it), std::make_move_iterator(raw_args.end()));
        }
        raw_args = std::move(args);
    }

    // Get binary path.
    auto it = raw_args.begin();
    parsed.binary = *it;
    ++it;

    if(parsed.kind == CommandKind::CK_COMPILE) {
        // If using a C/C++ binary, default file format to C/C++.
#if _WIN32
        // Do this to handle windows case insensitivity
        auto basename = fs::change_extension(fs::basename(parsed.binary), "").string();
        boost::algorithm::to_lower(basename);
#else
        auto basename = fs::basename(parsed.binary);
#endif
        if(c_aliases.count(basename)) {
            parsed.file_format = FileFormat::FF_C_SOURCE;
        } else if(cxx_aliases.count(basename)) {
            parsed.file_format = FileFormat::FF_CXX_SOURCE;
        } else {
            // Still let the extension decide if not a C/C++ alias.
            parsed.file_format = FileFormat::FF_EXT;
        }
    }

    // Pre-fill scan options.
    parsed.c_scan_options.insert(
        parsed.c_scan_options.end(),
        c_prepend_scan_options.begin(),
        c_prepend_scan_options.end());
    parsed.cxx_scan_options.insert(
        parsed.cxx_scan_options.end(),
        cxx_prepend_scan_options.begin(),
        cxx_prepend_scan_options.end());

    // Loop-independent flag for target recognition.
    bool has_target_exts = target_extensions.size() > 0;

    // Parse the options and update the state.
    while(it != raw_args.end()) {
        bool matched = false;
        for(const auto &p: options) {
            if(p->match(it, parsed)) {
                matched = true;
                break;
            }
        }
        if(matched) {
            // The option indicates this is not a command of interest.
            //   e.g. --help
            // So skip the process and just return.
            if(parsed.kind == CommandKind::CK_IGNORE) {
                LOG(info) << "Parsed kind: matched: " << command_kind_to_string(parsed.kind);
                break;
            }
            // Already advanced it, just continue;
            continue;
        }

        // Otherwise it should be unchanged and not equal to end().

        // Skip unhandled options. These should not have argFormat = space.
        if(option_prefix && ba::starts_with(*it, *option_prefix)) {
            parsed.pp_options.push_back(*it);
            ++it;
            continue;
        }

        // Otherwise check if this is source file.
        auto ext = fs::path(*it).extension().string();
        if(has_target_exts && !parsed.target.length() && target_extensions.count(ext)) {
            // If we see the first target-like file, set it as target.
            // Check target first for cmdlines like ar.
            parsed.target = get_full_path_str(*it, parsed.dir);
        } else if(source_extensions.count(ext)) {
            // Update file_format if by extension.
            auto format = parsed.file_format;
            if(format == FileFormat::FF_EXT) {
                format = source_extensions.find(ext)->second;
            }
            parsed.sources.emplace_back(get_full_path_str(*it, parsed.dir), format);
        } else if(it->length()) {
            parsed.pp_options.push_back(*it);
            LOG(warning) << "Unknown option '" << *it << "' in" << std::endl << ba::join(raw_args, " ");
        }
        ++it;
    }

    return boost::none;
}

std::vector<std::string> ToolProfile::get_preprocessing_options(
    const std::string &target,
    FileFormat format,
    const ParsedWorkItem &parsed
) const {
    bool single_source = parsed.sources.size() == 1;
    std::vector<std::string> res;
    const auto make_cmd_line = [&](
        const std::vector<std::string> &prepend,
        const std::vector<std::string> &append,
        const std::vector<std::string> &isystem,
        const std::vector<std::string> &include
    ) {
        // [prepend] + [path] + [inc] + [pp] + [append]
        res.insert(res.end(), prepend.begin(), prepend.end());
        if(sys_inc_path_option) {
            for(const auto &f: isystem) {
                auto opts = sys_inc_path_option->make_string_option(f);
                res.insert(res.end(), opts.begin(), opts.end());
            }
        }
        if(pre_include_option) {
            for(const auto &f: include) {
                auto opts = pre_include_option->make_string_option(f);
                res.insert(res.end(), opts.begin(), opts.end());
            }
        }
        if(single_source) {
            res.insert(
                res.end(),
                std::make_move_iterator(parsed.pp_options.begin()),
                std::make_move_iterator(parsed.pp_options.end()));
        } else {
            res.insert(res.end(), parsed.pp_options.begin(), parsed.pp_options.end());
        }
        res.insert(res.end(), append.begin(), append.end());
    };

    if(format == FileFormat::FF_C_SOURCE) {
        make_cmd_line(
            c_prepend_preprocessing_options,
            c_append_preprocessing_options,
            c_sys_inc_paths,
            c_pre_includes);
    } else {
        make_cmd_line(
            cxx_prepend_preprocessing_options,
            cxx_append_preprocessing_options,
            cxx_sys_inc_paths,
            cxx_pre_includes);
    }

    if(preprocess_option) {
        auto pp = preprocess_option->make_string_option("");
        res.insert(res.end(), pp.begin(), pp.end());
    }

    if(output_option) {
        auto out = output_option->make_string_option(target);
        res.insert(res.end(), out.begin(), out.end());
    }

    return res;
}
