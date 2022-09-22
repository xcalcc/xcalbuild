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
#include "parsed-work-item.hpp"

namespace xcal {

/// The type of option.
enum OptionType {
    OT_CMD = 0,
    OT_LANG = 1,
    OT_RESPFILE = 2,
    OT_DELETE = 3,
    OT_SCAN = 4,
    OT_PREPROCESS = 5,
    OT_OUTPUT = 6,
    OT_PRE_INCLUDE = 7,
    OT_SYS_INC_PATH = 8,
    OT_OTHER = 9,
};

boost::optional<OptionType> option_type_from_json(const nlohmann::json &type);
std::string option_type_to_string(OptionType type);

enum OptionArgFormat {
    OAF_ATTACHED = 0,
    OAF_SPACE = 1,
    OAF_EQUAL = 2,
};

boost::optional<OptionArgFormat> option_arg_format_from_json(const nlohmann::json &type);
std::string option_arg_format_to_string(OptionArgFormat type);

/**
 * @brief A command line option of a tool.
 *
 */
class Option {
public:
    /**
     * @brief Construct a new Option object
     *
     * @param json the json spec of the option.
     * @param default_prefix the default option prefix.
     */
    Option(const nlohmann::json &json, const char *default_prefix);

    virtual ~Option() = default;

    /**
     * @brief  Try to match the option pointed by it, and return the arg, if any.
     * If there are additional arguments, advace it accordingly.
     *
     * @param it the option iterator
     * @param arg the arg to return.
     * @return true if matches
     * @return false if not match, it should stay unchanged.
     */
    bool match_and_get_arg(std::vector<std::string>::iterator &it, std::string &arg) const;

    /**
     * @brief process the option with the arg.
     *
     * @param arg the arg to the option
     * @param parsed the state.
     * @return true if also copy options to pp_options.
     * @return false do not copy options to pp_options.
     */
    virtual bool process(const std::string &arg, ParsedWorkItem &parsed) const = 0;

    /**
     * @brief Try to match the option pointed by it.
     * If there are additional arguments, advace it accordingly.
     *
     * @param it the option iterator
     * @param parsed the state of option parsing
     * @return true if matches
     * @return false if not match, it and parsed should stay unchanged.
     */
    bool match(std::vector<std::string>::iterator &it, ParsedWorkItem &parsed) const;

    /**
     * @brief Whether this option has arg.
     *
     * @return true
     * @return false
     */
    bool has_arg() const;

    /**
     * @brief Make an option from the argument, basing on the argFormat specified.
     *
     * @param arg the option argument.
     * @return std::vector<std::string> the option and argument.
     */
    std::vector<std::string> make_string_option(const std::string &arg) const;

    /// The type of the option.
    const OptionType type;

    /// The default option prefix, used for optional argument handling.
    const char *default_prefix;

protected:
    /// The aliases of the option.
    std::set<std::string> aliases;

    /// The aliases, in original order.
    std::vector<std::string> ordered_aliases;

    /// The argument format.
    std::set<OptionArgFormat> argFormat;
};

/// An option that changes CommandKind.
class CmdOption: public Option {
public:
    CmdOption(const nlohmann::json &json, const char *default_prefix);

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
private:
    /// The command kind to switch to.
    CommandKind kind = CommandKind::CK_IGNORE;
};

/// An option that changes source file language.
class LangOption: public Option {
public:
    LangOption(const nlohmann::json &json, const char *default_prefix);

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
private:
    /// Map from argument value to source language.
    std::map<std::string, FileFormat> arg_to_lang;
};

/// A response file option.
class RespFileOption: public Option {
public:
    RespFileOption(const nlohmann::json &json, const char *default_prefix)
        : Option(json, default_prefix) {}

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
};

/// An option that should be delete from the preprocessing command line.
class DeleteOption: public Option {
public:
    DeleteOption(const nlohmann::json &json, const char *default_prefix)
        : Option(json, default_prefix) {}

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
};

/// An option that should be passed to xvsa scan.
class ScanOption: public Option {
public:
    ScanOption(const nlohmann::json &json, const char *default_prefix);

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
private:
    /// c scan option
    boost::optional<std::string> c_scan_option = boost::none;

    /// c++ scan option
    boost::optional<std::string> cxx_scan_option = boost::none;

    /// scan arg format, if any
    boost::optional<OptionArgFormat> scan_arg_format = boost::none;

    /// Map from argument value to c argument value.
    std::map<std::string, std::string> arg_to_c_scan;

    /// Map from argument value to cxx argument value.
    std::map<std::string, std::string> arg_to_cxx_scan;
};

/// An option that triggers preprocessing.
class PreprocessOption: public Option {
public:
    PreprocessOption(const nlohmann::json &json, const char *default_prefix)
        : Option(json, default_prefix) {}

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
};

/// An option that specifies the output target of the command line.
class OutputOption: public Option {
public:
    OutputOption(const nlohmann::json &json, const char *default_prefix)
        : Option(json, default_prefix) {}

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
};

/// An option that specifies the pre-include file.
class PreIncludeOption: public Option {
public:
    PreIncludeOption(const nlohmann::json &json, const char *default_prefix)
        : Option(json, default_prefix) {}


    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
};

/// An option that specifies the additional system include path.
class SystemIncludePathOption: public Option {
public:
    SystemIncludePathOption(const nlohmann::json &json, const char *default_prefix)
        : Option(json, default_prefix) {}

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
};

/// Only for handling other options with space-separated arguments.
class OtherOption: public Option {
public:
    OtherOption(const nlohmann::json &json, const char *default_prefix)
        : Option(json, default_prefix) {}

    bool process(const std::string &arg, ParsedWorkItem &parsed) const override;
};

/**
 * @brief Parse the option in the json object.
 *
 * @param op the input json.
 * @return Option * the parsed Option object, if any.
 */
Option *parse_option(const nlohmann::json &op, const char *default_prefix);

}
