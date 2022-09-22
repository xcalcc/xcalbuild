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

#include "validation.hpp"
#include "tool-profile.hpp"
#include "option.hpp"
#include "logger.hpp"
#include "path.hpp"

namespace ba = boost::algorithm;
namespace fs = boost::filesystem;
using json = nlohmann::json;

using namespace xcal;

constexpr const char *option_type_names[] = {
    "cmd",
    "language",
    "response",
    "delete",
    "scan",
    "preprocess",
    "output",
    "include",
    "isystem",
    "other",
};
CHECK_NAMES_SIZE(option_type_names, OptionType::OT_OTHER);

boost::optional<OptionType> xcal::option_type_from_json(const json &json) {
    auto type = json.get<std::string>();
    for(unsigned int i = 0; i <= (unsigned int)OptionType::OT_OTHER; ++i) {
        if(type.compare(option_type_names[i]) == 0) {
            return (OptionType)i;
        }
    }
    return boost::none;
}

std::string xcal::option_type_to_string(OptionType ty) {
    return option_type_names[(unsigned int)ty];
}

constexpr const char *option_arg_format_names[] = {
    "attached",
    "space",
    "equal",
};
CHECK_NAMES_SIZE(option_arg_format_names, OptionArgFormat::OAF_EQUAL);

boost::optional<OptionArgFormat> xcal::option_arg_format_from_json(const json &json) {
    auto format = json.get<std::string>();
    for(unsigned int i = 0; i <= (unsigned int)OptionArgFormat::OAF_EQUAL; ++i) {
        if(format.compare(option_arg_format_names[i]) == 0) {
            return (OptionArgFormat)i;
        }
    }
    return boost::none;
}

std::string xcal::option_arg_format_to_string(OptionArgFormat format) {
    return option_arg_format_names[(unsigned int)format];
}

Option::Option(const nlohmann::json &json, const char *default_prefix)
    : type(*option_type_from_json(json["type"].get<std::string>())),
      default_prefix(default_prefix)
{
    validate_json(json, "aliases", JSONType::JT_ARRAY);

    // Parse aliases.
    for(const auto &alias: json["aliases"]) {
        validate_json(alias, "aliases", JSONType::JT_STRING_ARRAY, /*lookup_key*/false);
        auto a = alias.get<std::string>();
        aliases.insert(a);
        ordered_aliases.push_back(std::move(a));
    }

    if(json.count("argFormat")) {
        validate_json(json, "argFormat", JSONType::JT_ARRAY);
        for(const auto &format: json["argFormat"]) {
            validate_json(format, "argFormat", JSONType::JT_STRING_ARRAY, /*lookup_key*/false);
            auto oaf = option_arg_format_from_json(format);
            validate_optional(oaf, "argFormat", json);
            argFormat.insert(*oaf);
        }
    }
}

bool Option::match_and_get_arg(std::vector<std::string>::iterator &it, std::string &arg) const {
    // No arg.
    if (!argFormat.size()) {
        if (aliases.count(*it)) {
            ++it;
            return true;
        } else {
            return false;
        }
    }

    if (argFormat.count(OptionArgFormat::OAF_SPACE)) {
        if (aliases.count(*it)) {
            ++it;
            if(!default_prefix || !ba::starts_with(*it, default_prefix)) {
                // Only add arg if:
                //   * no `optionPrefix` in the profile, or
                //   * the arg does not start with the prefix
                // TODO: actually add a optional-space arg format.
                arg = *it;
                ++it;
            }
            return true;
        }
    }

    if (argFormat.count(OptionArgFormat::OAF_EQUAL)) {
        std::vector<std::string> v;
        ba::split(v, *it, boost::is_any_of("="));
        if (v.size() == 2 && aliases.count(v[0])) {
            ++it;
            arg = v[1];
            return true;
        }
    }

    if (argFormat.count(OptionArgFormat::OAF_ATTACHED)) {
        for (const auto &alias: ordered_aliases) {
            if (ba::starts_with(*it, alias)) {
                arg = it->substr(alias.length());
                ++it;
                return true;
            }
        }
    }

    return false;
}


bool Option::match(std::vector<std::string>::iterator &it, ParsedWorkItem &parsed) const {
    auto orig_it = it;
    std::string arg;
    bool matched = Option::match_and_get_arg(it, arg);

    if(matched) {
        if(process(arg, parsed)) {
            parsed.pp_options.insert(parsed.pp_options.end(), orig_it, it);
        }
    }

    return matched;
}


std::vector<std::string> Option::make_string_option(const std::string &arg) const {
    std::vector<std::string> res;
    // Pick the first option alias, the list can't be empty after validation.
    auto option_it = ordered_aliases.begin();

    auto format_it = argFormat.begin();

    if(format_it == argFormat.end()) {
        res.push_back(*option_it);
        return res;
    }

    switch(*format_it) {
        case OptionArgFormat::OAF_ATTACHED:
            res.push_back((*option_it) + arg);
            break;
        case OptionArgFormat::OAF_SPACE:
            res.push_back(*option_it);
            res.push_back(arg);
            break;
        case OptionArgFormat::OAF_EQUAL:
            res.push_back((*option_it) + "=" + arg);
            break;
        default:
            BOOST_UNREACHABLE_RETURN(res); //GCOVR_EXCL_LINE
            break;
    }

    return res;
}

bool Option::has_arg() const {
    return argFormat.size() != 0;
}

CmdOption::CmdOption(const nlohmann::json &json, const char *default_prefix): Option(json, default_prefix) {
    validate_json(json, "kind", JSONType::JT_STRING);
    auto kind_opt = command_kind_from_json(json["kind"]);
    validate_optional(kind_opt, "kind", json);
    kind = *kind_opt;
}

bool CmdOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    (void)arg;
    parsed.kind = kind;
    return true;
}

LangOption::LangOption(const nlohmann::json &json, const char *default_prefix): Option(json, default_prefix) {
    validate_json(json, "argValues", JSONType::JT_OBJECT);
    for(const auto &i: json["argValues"].items()) {
        auto val = file_format_from_string(i.value());
        validate_optional(val, i.key(), json["argValues"]);
        arg_to_lang.insert(std::make_pair(i.key(), *val));
    }
}

bool LangOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    auto it = arg_to_lang.find(arg);
    if(it != arg_to_lang.end()) {
        parsed.file_format = it->second;
    }
    // Remove language options from pp command line,
    // since the information is already captured in file format.
    return false;
}

bool RespFileOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    (void)arg;
    (void)parsed;
    return false;
}

bool DeleteOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    (void)arg;
    (void)parsed;
    return false;
}

ScanOption::ScanOption(const nlohmann::json &json, const char *default_prefix): Option(json, default_prefix) {
    const auto fill_map = [&](const char *key, std::map<std::string, std::string> &m) {
        if(json.count(key)) {
            validate_json(json, key, JSONType::JT_OBJECT);
            for(const auto &i: json[key].items()) {
                if(!i.key().length()) {
                    LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Keys should not be empty strings in " << json[key].dump();
                    throw IncorrectProfileException();
                }
                validate_json(i.value(), key, JSONType::JT_STRING, /*lookup_key*/false);
                m.insert(std::make_pair(i.key(), i.value()));
            }
        }
    };
    fill_map("cArgValues", arg_to_c_scan);
    fill_map("cxxArgValues", arg_to_cxx_scan);

    if(json.count("cScanOption")) {
        validate_json(json, "cScanOption", JSONType::JT_STRING);
        c_scan_option = json["cScanOption"].get<std::string>();
    }
    if(json.count("cxxScanOption")) {
        validate_json(json, "cxxScanOption", JSONType::JT_STRING);
        cxx_scan_option = json["cxxScanOption"].get<std::string>();
    }

    if(json.count("scanArgFormat")) {
        validate_json(json, "scanArgFormat", JSONType::JT_STRING);
        auto oaf = option_arg_format_from_json(json["scanArgFormat"]);
        validate_optional(oaf, "scanArgFormat", json);
        scan_arg_format = oaf;
    }
}

bool ScanOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    // If no scan option ScanOptioned, default to the current alias.
    auto c_option = c_scan_option ? *c_scan_option : ordered_aliases[0];
    auto cxx_option = cxx_scan_option ? *cxx_scan_option : ordered_aliases[0];

    // If no arg, just append the option.
    if(scan_arg_format == boost::none) {
        parsed.c_scan_options.push_back(c_option);
        parsed.cxx_scan_options.push_back(cxx_option);
        return true;
    }

    const auto add_option = [&](
        const std::map<std::string, std::string> &m,
        const std::string &option,
        std::vector<std::string> &options
    ) {
        // Check if the arg value is known.
        auto scan_it = m.find(arg);

        auto scan_arg = arg;
        if(scan_it != m.end()) {
            scan_arg = scan_it->second;
        }
        /* This is pretty noisy.
        else {
            LOG(info) << "Arg value '" << arg << "' not mapped for scan option '" << option << "', use as is";
        }
        */

        switch(*scan_arg_format) {
            case OptionArgFormat::OAF_SPACE: {
                options.push_back(option);
                options.push_back(scan_arg);
                break;
            }
            case OptionArgFormat::OAF_ATTACHED:
                options.push_back(option + scan_arg);
                break;
            case OptionArgFormat::OAF_EQUAL:
                options.push_back(option + "=" + scan_arg);
                break;
        }
    };
    add_option(arg_to_c_scan, c_option, parsed.c_scan_options);
    add_option(arg_to_cxx_scan, cxx_option, parsed.cxx_scan_options);
    return true;
}

bool PreprocessOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    (void)arg;
    (void)parsed;
    // TODO: might want to handle this case.
    return false;
}

bool OutputOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    parsed.target = get_full_path_str(arg, parsed.dir);
    return false;
}

bool PreIncludeOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    (void)arg;
    (void)parsed;
    return true;
}

bool SystemIncludePathOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    (void)arg;
    (void)parsed;
    return true;
}

bool OtherOption::process(const std::string &arg, ParsedWorkItem &parsed) const {
    (void)arg;
    (void)parsed;
    return true;
}

Option *xcal::parse_option(const nlohmann::json &op, const char *default_prefix) try {
    validate_json(op, "type", JSONType::JT_STRING);
    auto ty = option_type_from_json(op["type"]);
    validate_optional(ty, "type", op);
    switch(*ty) {
        case OptionType::OT_CMD: return new CmdOption(op, default_prefix);
        case OptionType::OT_LANG: return new LangOption(op, default_prefix);
        case OptionType::OT_RESPFILE: return new RespFileOption(op, default_prefix);
        case OptionType::OT_DELETE: return new DeleteOption(op, default_prefix);
        case OptionType::OT_SCAN: return new ScanOption(op, default_prefix);
        case OptionType::OT_PREPROCESS: return new PreprocessOption(op, default_prefix);
        case OptionType::OT_OUTPUT: return new OutputOption(op, default_prefix);
        case OptionType::OT_PRE_INCLUDE: return new PreIncludeOption(op, default_prefix);
        case OptionType::OT_SYS_INC_PATH: return new SystemIncludePathOption(op, default_prefix);
        case OptionType::OT_OTHER: return new OtherOption(op, default_prefix);
    }
    BOOST_UNREACHABLE_RETURN(nullptr); //GCOVR_EXCL_LINE
} catch (IncorrectProfileException &e) {
    return nullptr;
}