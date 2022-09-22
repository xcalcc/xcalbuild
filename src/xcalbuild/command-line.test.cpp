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

#include "command-line.hpp"
#include "logger.hpp"
#include "error-code.hpp"

using namespace xcal;
namespace fs = boost::filesystem;

bool init_unit_test() {
    auto log_file = fs::system_complete(fs::path("command-line-test.log")).string();
    std::string trace_id = "TRACEID";
    std::string span_id = "SPANID";
    init_logger(log_file, trace_id, span_id, /*debug*/false, /*local_log*/true);
    return true;
}

void test_cmdline_ec(int argc, char *argv[], const boost::optional<ErrorCode> &ec_opt) {
    CommandLine cl;
    BOOST_TEST(cl.parse(argc, (char **)argv) == ec_opt);
}

BOOST_AUTO_TEST_CASE( test_missing_builddir ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "--outputdir", "..", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_INCORRECT_COMMAND_LINE);
}

BOOST_AUTO_TEST_CASE( test_missing_outputdir ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "--builddir", ".", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_INCORRECT_COMMAND_LINE);
}

BOOST_AUTO_TEST_CASE( test_version ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "--version", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_NONE);
}

BOOST_AUTO_TEST_CASE( test_help ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "--help", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_NONE);
}

BOOST_AUTO_TEST_CASE( test_empty_build_command ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "-i", ".", "-o", "."};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_INCORRECT_COMMAND_LINE);
}

BOOST_AUTO_TEST_CASE( test_invalid_builddir ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "-i", "/a/b/c/d/e/f/g", "-o", ".", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_INCORRECT_COMMAND_LINE);
}

BOOST_AUTO_TEST_CASE( test_invalid_outputdir ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "-o", "/a/b/c/d/e/f/g", "-i", ".", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_INCORRECT_COMMAND_LINE);
}

BOOST_AUTO_TEST_CASE( test_invalid_profile ) {
    // Parse command line.
    auto tmp_dir = fs::system_complete("command-line-test-tmp");
    auto profile_path = tmp_dir / "profiles" / "gnu";
    fs::create_directories(profile_path);
    fs::create_directories(tmp_dir / "bin");
    auto xcalbuild_path_str = (tmp_dir / "bin" / "xcalbuild").string();
    const char *argv[] = {xcalbuild_path_str.c_str(), "-i", ".", "-o", ".", "--profile", "clang", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_INCORRECT_COMMAND_LINE);
    fs::remove_all(tmp_dir);
}

BOOST_AUTO_TEST_CASE( test_invalid_tracing_method ) {
    // Parse command line.
    const char *argv[] = {"xcalbuild", "-i", "..", "-o", "..", "-t", "none", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    test_cmdline_ec(argc, (char **)argv, EC_INCORRECT_COMMAND_LINE);
}

BOOST_AUTO_TEST_CASE( test_getters ) {
    // Parse command line.
    auto tmp_dir = fs::system_complete("command-line-test-tmp");
    auto profile_path = tmp_dir / "profiles" / "clang";
    fs::create_directories(profile_path);
    auto xcalbuild_path_str = (tmp_dir / "bin" / "xcalbuild").string();
    const char *argv[] = {
        xcalbuild_path_str.c_str(),
        "--builddir", ".",
        "--outputdir", "..",
        "--prebuild", "\"make clean\"",
        "--debug",
        "--preprocessor", "cpp",
        "--tracing_method", "static",
        "--parallel", "4",
        "--profile", "clang",
        "--", "make", "j4"};
    int argc = sizeof(argv) / sizeof(const char *);
    CommandLine cl;
    BOOST_TEST(cl.parse(argc, (char **)argv) == boost::none);
    BOOST_TEST(cl.get_tool_root() == tmp_dir);
    BOOST_TEST(cl.get_build_dir() == ".");
    BOOST_TEST(cl.get_output_dir() == "..");
    BOOST_TEST(cl.get_prebuild() == "\"make clean\"");
    BOOST_TEST(cl.is_debug());
    BOOST_TEST(cl.get_preprocessor() == "cpp");
    BOOST_TEST(cl.get_tracing_method() == TracingMethod::STRACE);
    BOOST_TEST(cl.get_parallelism() == 4);
    BOOST_TEST(cl.get_toolchain_profile() == profile_path);

    auto &build_commands = cl.get_build_commands();
    std::vector<std::string> expected_commands = {"make", "j4"};
    BOOST_TEST(build_commands == expected_commands);
    fs::remove_all(tmp_dir);
}

BOOST_AUTO_TEST_CASE( test_default ) {
    // Parse command line.
    auto tmp_dir = fs::system_complete("command-line-test-tmp");
    auto profile_path = tmp_dir / "profiles" / "linux-auto";
    fs::create_directories(profile_path);
    auto xcalbuild_path_str = (tmp_dir / "bin" / "xcalbuild").string();
    const char *argv[] = {
        xcalbuild_path_str.c_str(),
        "--builddir", ".",
        "--outputdir", "..",
        "--prebuild", "\"make clean\"",
        "--", "make", "j4"};
    int argc = sizeof(argv) / sizeof(const char *);
    CommandLine cl;
    BOOST_TEST(cl.parse(argc, (char **)argv) == boost::none);
    BOOST_TEST(!cl.is_debug());
    BOOST_TEST(cl.get_preprocessor() == "default");
    BOOST_TEST(cl.get_tracing_method() == TracingMethod::BEAR);
    BOOST_TEST(cl.get_parallelism() == 1);
    BOOST_TEST(cl.get_toolchain_profile() == profile_path);
    fs::remove_all(tmp_dir);
}

BOOST_AUTO_TEST_CASE( test_config ) {
    // Parse command line.
    auto tmp_dir = fs::system_complete("command-line-test-tmp");
    auto profile_path = tmp_dir / "profiles" / "gnu";
    fs::create_directories(profile_path);
    auto xcalbuild_path_str = (tmp_dir / "bin" / "xcalbuild").string();
    const char *argv[] = {
        xcalbuild_path_str.c_str(),
        "--builddir", ".",
        "--outputdir", "..",
        "--prebuild", "\"make clean\"",
        "--", "make", "j4"};
    int argc = sizeof(argv) / sizeof(const char *);
    CommandLine cl;
    cl.parse(argc, (char **)argv);

    {
        fs::ofstream config { tmp_dir / "config" };
        config <<R"(
[linux]
VERSION
        )";
        config.close();
    }
    BOOST_TEST(cl.parse_config() == EC_INCORRECT_CONFIG_FILE);

    {
        fs::ofstream config { tmp_dir / "config" };
        config <<R"(
[linux]
VERSION=0.1.3
CDB_NAME=cdb.json
PREPROCESS=pp
SOURCE_FILES=srcs.json
CLANG_OPTIONS=-D__clang__
[PROPERTY_KEY]
version=
project=
vcs_tool=
        )";
        config.close();
    }
    BOOST_TEST(cl.parse_config() == boost::none);
    BOOST_TEST(cl.get_cdb_name() == "cdb.json");
    BOOST_TEST(cl.get_preprocess_dir_name() == "pp");
    BOOST_TEST(cl.get_source_list_file_name() == "srcs.json");
    BOOST_TEST(cl.get_config("linux.CLANG_OPTIONS") == boost::optional<std::string>("-D__clang__"));
    BOOST_TEST(cl.get_config("linux.NOT_EXIST") == boost::none);
    BOOST_TEST(cl.get_properties_template().count("version"));
    BOOST_TEST(!cl.get_properties_template().count("ld_flags"));

    {
        fs::ofstream config { tmp_dir / "config" };
        config <<R"(
[linux]
VERSION=0.1.3
PREPROCESS=
        )";
        config.close();
    }
    BOOST_TEST(cl.parse_config() == boost::none);
    BOOST_TEST(cl.get_cdb_name() == "compile_commands.json");
    BOOST_TEST(cl.get_preprocess_dir_name() == "preprocess");
    BOOST_TEST(cl.get_source_list_file_name() == "source_files.json");
    fs::remove_all(tmp_dir);
}
