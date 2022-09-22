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

#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/optional.hpp>

#include <nlohmann/json.hpp>

#include "toolchain-profile.hpp"
#include "logger.hpp"
#include "error-code.hpp"

using namespace xcal;
namespace fs = boost::filesystem;
using json = nlohmann::json;

bool init_unit_test() {
    auto log_file = fs::system_complete(fs::path("toolchain-profile-test.log")).string();
    std::string trace_id = "TRACEID";
    std::string span_id = "SPANID";
    init_logger(log_file, trace_id, span_id, /*debug*/false, /*local_log*/true);
    return true;
}

static void write_file(fs::path path, const char *content) {
    fs::ofstream ofs{ path };
    ofs << content << std::endl;
    ofs.close();
}

struct TestProfileDir {
    TestProfileDir(const char *profile_content) {
        path = fs::system_complete(fs::path("toolchain-profile-test-tmp"));
        fs::remove_all(path);
        fs::create_directories(path);
        write_file(path / "profile.json", profile_content);
    }
    ~TestProfileDir() {
        fs::remove_all(path);
    }
    fs::path path;
};

BOOST_AUTO_TEST_CASE(test_load_binaries_with_default) {
    TestProfileDir dir(R"({
    "tools": [
        {
            "aliases": ["cc1", "cc1plus"],
            "profile": "./gcc.json"
        },
        {
            "profile": "./as.json"
        },
        {
            "profile": "./ld.json"
        }
    ]
})");
    write_file(dir.path / "gcc.json", R"({
    "aliases": ["gcc", "g++"],
    "defaultCommandKind": "compile",
    "options": [
        {"aliases": ["@"], "argFormat" : ["attached"], "type": "response"}
    ]
})");
    write_file(dir.path / "as.json", R"({
    "aliases": ["as"],
    "defaultCommandKind": "assemble",
    "options": [
        {"aliases": ["@"], "argFormat" : ["attached"], "type": "response"}
    ]
})");
    write_file(dir.path / "ld.json", R"({
    "aliases": ["ld"],
    "defaultCommandKind": "link",
    "options": [
        {"aliases": ["@"], "argFormat" : ["attached"], "type": "response"}
    ]
})");
    ToolchainProfile profile;
    profile.load(dir.path);
    auto binaries = profile.get_binaries_to_trace();
    std::vector<std::string> actual_names;
    json actual_configs;
    for(const auto &p: binaries) {
        actual_names.push_back(p.first);
        actual_configs.push_back(p.second);
    }
    std::vector<std::string> expected_names = {"as", "cc1", "cc1plus", "ld"};
    BOOST_TEST(actual_names == expected_names);
    BOOST_TEST(actual_configs == R"([
        {"binary": "as", "responseFileArgs": [{"argument": "@", "argFormat" : ["attached"]}]},
        {"binary": "cc1", "responseFileArgs": [{"argument": "@", "argFormat" : ["attached"]}]},
        {"binary": "cc1plus", "responseFileArgs": [{"argument": "@", "argFormat" : ["attached"]}]},
        {"binary": "ld", "responseFileArgs": [{"argument": "@", "argFormat" : ["attached"]}]}
    ])"_json);
}

BOOST_AUTO_TEST_CASE(test_load_toolchain_profile) {
    TestProfileDir dir("");
    write_file(dir.path / "gcc.json", R"({
    "aliases": ["gcc", "g++"],
    "defaultCommandKind": "compile",
    "options": [
        {"aliases": ["@"], "argFormat" : ["attached"], "type": "response"}
    ]
})");
    write_file(dir.path / "bad-gcc.json", "{}");
    std::vector<json> fails = {
        "{}"_json,
        R"({"tools": "a"})"_json,
        R"({"tools": null})"_json,
        R"({"tools": {}})"_json,
        R"({"tools": ["a"]})"_json,
        R"({"tools": [[]]})"_json,
        R"({"tools": [{}]})"_json,
        R"({"tools": [{"profile": []}]})"_json,
        R"({"tools": [{"profile": null}]})"_json,
        R"({"tools": [{"profile": "./gcc.json", "aliases": "gcc"}]})"_json,
        R"({"tools": [{"profile": "./gcc.json", "aliases": [{}]}]})"_json,
        R"({"tools": [{"profile": "./bad-gcc.json"}]})"_json,
    };

    ToolchainProfile profile;
    BOOST_TEST(profile.load(dir.path) == EC_INCORRECT_TOOLCHAIN_PROFILE);
    for(const auto &fail: fails) {
        BOOST_TEST(profile.load(fail, dir.path) == EC_INCORRECT_TOOLCHAIN_PROFILE);
    }
}

BOOST_AUTO_TEST_CASE(test_load_tool_profile) {
    ToolProfile profile;
    TestProfileDir dir("");
    BOOST_TEST(profile.load(dir.path / "profile.json") == EC_INCORRECT_TOOLCHAIN_PROFILE);

    std::vector<json> fails;
    std::vector<json> passes;
#define ADD_FAIL(x) fails.push_back(x##_json)
#define ADD_PASS(x) passes.push_back(x##_json)

    // The order of parsing is important.
    ADD_FAIL("{}");
    ADD_FAIL(R"({"aliases": "a"})");
    ADD_FAIL(R"({"aliases": []})");
    ADD_FAIL(R"({"aliases": [{}]})");
    ADD_FAIL(R"({"aliases": ["cc"]})");

    ADD_FAIL(R"({"aliases": ["cc"], "cAliases": "a"})");
    ADD_FAIL(R"({"aliases": ["cc"], "cAliases": [{}]})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cAliases": ["cc"]})");

    ADD_FAIL(R"({"aliases": ["cc"], "cxxAliases": "a"})");
    ADD_FAIL(R"({"aliases": ["cc"], "cxxAliases": [{}]})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cxxAliases": ["cc"]})");

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": {}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "make"})");

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "optionPrefix": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "optionPrefix": {}})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "optionPrefix": "-"})");

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": "a"})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": {}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": ["a"]})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}]})");
    // Actual option parsing to be tested in test_parse_options.

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "sourceExtensions": ""})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "sourceExtensions": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "sourceExtensions": {}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "sourceExtensions": {"": []}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "sourceExtensions": {"c": ""}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "sourceExtensions": {"c": []}})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "sourceExtensions": {"c": ["x"]}})");

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "targetExtensions": ""})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "targetExtensions": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "targetExtensions": {}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "targetExtensions": {"": []}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "targetExtensions": {"c": ""}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "targetExtensions": {"c": []}})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "targetExtensions": {"c": ["x"]}})");

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": ""})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": {}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "x"}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "x", "string": []}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "x", "string": {}}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "x", "string": ""}]})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "", "string": "x"}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "x", "regex": []}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "x", "regex": {}}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "x", "regex": ""}]})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "textSubstitutions": [{"replacement": "", "regex": "x"}]})");

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "cPrependPreprocessingOptions": ""})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "cPrependPreprocessingOptions": {}})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "cPrependPreprocessingOptions": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "cPrependPreprocessingOptions": [{}]})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "cPrependPreprocessingOptions": [""]})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "cPrependPreprocessingOptions": ["x"]})");
    // The rest of actionable are the same.

    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": ""})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": []})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"":""}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":[]}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":{}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":{"":[]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":{"x":[]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":{"x":[""]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":{"x":[{}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":{"x":[{"config":""}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}],
                 "probeCMacros": {"x":{"x":[{"config":"optionPrefix"}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions"}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": []}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": ""}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "x"}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "prepend"}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "prepend"}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "prepend", "value": ""}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "prepend", "value": {}}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "prepend", "value": []}]}}})");
    ADD_FAIL(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "prepend", "value": [""]}]}}})");
    ADD_PASS(R"({"aliases": ["cc"], "defaultCommandKind": "compile", "options": [{"aliases": ["-x"], "type": "other"}], "cPrependPreprocessingOptions": [],
                 "probeCMacros": {"x":{"x":[{"config":"cPrependPreprocessingOptions", "action": "prepend", "value": ["x"]}]}}})");
    // probeCxxMacros should be the same.

    for(const auto &fail: fails) {
        BOOST_TEST(profile.load(fail) == EC_INCORRECT_TOOLCHAIN_PROFILE, "Should not parse " << fail.dump());
    }
    for(const auto &pass: passes) {
        BOOST_TEST(profile.load(pass) == boost::none, "Failed to parse " << pass.dump());
    }
}

BOOST_AUTO_TEST_CASE(test_no_tool_profile_for_incorrect_work_item) {
    TestProfileDir dir(R"({
    "tools": [
        {
            "aliases": ["cc1", "cc1plus"],
            "profile": "./gcc.json"
        }
    ]
})");
    write_file(dir.path / "gcc.json", R"({
        "aliases": ["gcc"],
        "defaultCommandKind": "compile",
        "options": [{"aliases": ["-x"], "type": "other"}]
    })");
    ToolchainProfile profile;
    BOOST_TEST(profile.load(dir.path) == boost::none);
    auto p = profile.get_tool_profile("{}"_json);
    BOOST_TEST(p == nullptr);
    p = profile.get_tool_profile(R"({"arguments":[]})"_json);
    BOOST_TEST(p == nullptr);
    p = profile.get_tool_profile(R"({"arguments":["/usr/bin/gcc"]})"_json);
    BOOST_TEST(p == nullptr);
}

BOOST_AUTO_TEST_CASE(test_enum_round_trips) {
    for(unsigned int i = 0; i <= CommandKind::CK_IGNORE; ++i) {
        BOOST_TEST(command_kind_from_json(command_kind_to_string((CommandKind)i)) == (CommandKind)i);
    }
    for(unsigned int i = 0; i <= FileFormat::FF_EXT; ++i) {
        BOOST_TEST(file_format_from_string(file_format_to_string((FileFormat)i)) == (FileFormat)i);
    }
    for(unsigned int i = 0; i <= OptionType::OT_OTHER; ++i) {
        BOOST_TEST(option_type_from_json(option_type_to_string((OptionType)i)) == (OptionType)i);
    }
    for(unsigned int i = 0; i <= OptionArgFormat::OAF_EQUAL; ++i) {
        BOOST_TEST(option_arg_format_from_json(option_arg_format_to_string((OptionArgFormat)i)) == (OptionArgFormat)i);
    }
}

BOOST_AUTO_TEST_CASE(test_enum_from_bad_inputs) {
    auto j = "[\"x\"]"_json;
    BOOST_TEST(command_kind_from_json(j[0]) == boost::none);
    BOOST_TEST(file_format_from_string("") == boost::none);
    BOOST_TEST(option_type_from_json(j[0]) == boost::none);
    BOOST_TEST(option_arg_format_from_json(j[0]) == boost::none);
}


BOOST_AUTO_TEST_CASE(test_parse_options) {
    std::vector<json> fails;
    std::vector<json> passes;
#define ADD_FAIL(x) fails.push_back(x##_json)
#define ADD_PASS(x) passes.push_back(x##_json)

    // The order of parsing is important.
    ADD_FAIL("{}");
    ADD_FAIL(R"({"aliases": "a"})");
    ADD_FAIL(R"({"aliases": []})");
    ADD_FAIL(R"({"aliases": [{}]})");
    ADD_FAIL(R"({"aliases": ["-c"]})");

    ADD_FAIL(R"({"aliases": ["-c"], "type": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": ""})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "x"})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "other"})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "other", "argFormat": ""})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "other", "argFormat": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "other", "argFormat": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "other", "argFormat": [""]})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "other", "argFormat": ["x"]})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "other", "argFormat": ["space"]})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "other", "argFormat": ["space", "equal"]})");

    ADD_FAIL(R"({"aliases": ["-c"], "type": "cmd"})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "cmd", "kind": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "cmd", "kind": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "cmd", "kind": ""})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "cmd", "kind": "x"})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "cmd", "kind": "compile"})");

    ADD_FAIL(R"({"aliases": ["-c"], "type": "language"})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "language", "argValues": ""})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "language", "argValues": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "language", "argValues": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "language", "argValues": {"":""}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "language", "argValues": {"x":""}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "language", "argValues": {"x":"x"}})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "language", "argValues": {"x":"c"}})");

    ADD_PASS(R"({"aliases": ["-c"], "type": "scan"})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cScanOption": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cScanOption": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cScanOption": ""})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "scan", "cScanOption": "x"})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxScanOption": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxScanOption": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxScanOption": ""})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "scan", "cxxScanOption": "x"})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "scanArgFormat": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "scanArgFormat": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "scanArgFormat": ""})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "scan", "scanArgFormat": "space"})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cArgValues": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cArgValues": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cArgValues": {"":""}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cArgValues": {"x":[]}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cArgValues": {"x":{}}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cArgValues": {"x":""}})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "scan", "cArgValues": {"x":"x"}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxArgValues": []})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxArgValues": {}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxArgValues": {"":""}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxArgValues": {"x":[]}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxArgValues": {"x":{}}})");
    ADD_FAIL(R"({"aliases": ["-c"], "type": "scan", "cxxArgValues": {"x":""}})");
    ADD_PASS(R"({"aliases": ["-c"], "type": "scan", "cxxArgValues": {"x":"x"}})");

    for(const auto &fail: fails) {
        auto opt = std::unique_ptr<Option>(parse_option(fail, "-"));
        BOOST_TEST(opt.get() == nullptr, "Should not parse " << fail.dump());
    }
    for(const auto &pass: passes) {
        auto opt = std::unique_ptr<Option>(parse_option(pass, "-"));
        BOOST_TEST(opt.get() != nullptr, "Failed to parse " << pass.dump());
    }
}

BOOST_AUTO_TEST_CASE(test_option_make_string_option) {
    const auto check_string_opt = [&](json option, std::vector<std::string> expected) {
        auto opt = parse_option(option, "-");
        BOOST_TEST(opt, "Failed to parse " << option.dump());
        auto actual = opt->make_string_option("x");
        BOOST_TEST(actual == expected);
    };
    check_string_opt(R"({"aliases":["-c"], "type": "other"})"_json, {"-c"});
    check_string_opt(R"({"aliases":["-c"], "type": "other", "argFormat": ["attached"]})"_json, {"-cx"});
    check_string_opt(R"({"aliases":["-c"], "type": "other", "argFormat": ["space"]})"_json, {"-c","x"});
    check_string_opt(R"({"aliases":["-c"], "type": "other", "argFormat": ["equal"]})"_json, {"-c=x"});
    // First in set.
    check_string_opt(R"({"aliases":["-c"], "type": "other", "argFormat": ["equal", "space"]})"_json, {"-c", "x"});
}

BOOST_AUTO_TEST_CASE(test_option_has_arg) {
    BOOST_TEST(parse_option(R"({"aliases":["-c"], "type": "other"})"_json, "-")->has_arg() == false);
    BOOST_TEST(parse_option(R"({"aliases":["-c"], "type": "other", "argFormat": ["attached"]})"_json, "-")->has_arg() == true);
}

BOOST_AUTO_TEST_CASE(test_option_process) {
    // Some options not covered by profile tests.
    ParsedWorkItem parsed;
    std::string arg = "m";
    auto opt = parse_option(R"({"aliases":["-c"], "type": "response", "argFormat": ["space"]})"_json, "-");
    BOOST_TEST(opt->process(arg, parsed) == false);
    opt = parse_option(R"({"aliases":["-c"], "type": "preprocess"})"_json, "-");
    BOOST_TEST(opt->process(arg, parsed) == false);
    opt = parse_option(R"({"aliases":["-c"], "type": "isystem"})"_json, "-");
    BOOST_TEST(opt->process(arg, parsed) == true);
    opt = parse_option(R"({"aliases":["-c"], "type": "include"})"_json, "-");
    BOOST_TEST(opt->process(arg, parsed) == true);

    opt = parse_option(R"({"aliases":["-c"], "type": "scan", "cScanOption": "-x", "scanArgFormat": "space", "argFormat": ["space"]})"_json, "-");
    BOOST_TEST(opt->process(arg, parsed) == true);
    std::vector<std::string> expected = {"-x", "m"};
    BOOST_TEST(parsed.c_scan_options == expected);

    opt = parse_option(R"({"aliases":["-c"], "type": "scan", "cScanOption": "-x", "scanArgFormat": "attached", "argFormat": ["attached"]})"_json, "-");
    parsed.c_scan_options.clear();
    opt->process(arg, parsed);
    BOOST_TEST(parsed.c_scan_options[0] == "-xm");
}

BOOST_AUTO_TEST_CASE(test_load_resp_file) {
    TestProfileDir dir(R"({"tools": [{"profile": "./gcc.json"}]})");
    write_file(dir.path / "gcc.json", R"({
    "aliases": ["gcc", "g++"],
    "defaultCommandKind": "compile",
    "optionPrefix": "-",
    "options": [
        {"aliases": ["@"], "argFormat" : ["attached"], "type": "response"}
    ]
    })");
    write_file(dir.path / "tmp", R"(-c -g
    -m)");
    ToolchainProfile profile;
    BOOST_TEST(profile.load(dir.path) == boost::none);

    auto work_item = R"({
        "directory": "/work",
        "arguments": ["gcc", "@tmp", "-k", "-p"]
    })"_json;

    const ToolProfile *tp = profile.get_tool_profile(work_item);
    ParsedWorkItem parsed;

    BOOST_TEST(tp->parse_work_item(work_item, parsed) == boost::none);
    std::vector<std::string> expected = {"-k", "-p"};
    BOOST_TEST(parsed.pp_options == expected);
    parsed.pp_options.clear();

    work_item["arguments"][1] = std::string("@") + (dir.path / "tmp").string();
    BOOST_TEST(tp->parse_work_item(work_item, parsed) == boost::none);
    expected = {"-c", "-g", "-m", "-k", "-p"};
    BOOST_TEST(parsed.pp_options == expected);
}