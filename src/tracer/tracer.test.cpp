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
#include <boost/process.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>

#include <nlohmann/json.hpp>

#include <iostream>

#include "command-line.hpp"
#include "toolchain-profile.hpp"
#include "tracer.hpp"
#include "logger.hpp"
#include "error-code.hpp"

using namespace xcal;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
namespace bp = boost::process;
using json = nlohmann::json;

bool init_unit_test() {
    auto log_file = fs::system_complete(fs::path("tracer-test.log")).string();
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

struct TestTracerDir {
    TestTracerDir() {
        path = fs::system_complete(fs::path("tracer-test-tmp"));
        fs::remove_all(path);
        fs::create_directories(path);
        write_file(path / "Makefile","all:\n\tgcc -o a.out test.c\n");
        write_file(path / "test.c", "int main(){}\n");
    }
    ~TestTracerDir() {
        //fs::remove_all(path);
    }
    fs::path path;
};

static json test_trace(TracingMethod tracing_method) {
    // Test when there is xcalbuild, make and gcc.
    auto xcalbuild_path = bp::search_path("xcalbuild").string();
    if(!(bp::search_path("make").string().length() &&
         bp::search_path("gcc").string().length() &&
         xcalbuild_path.length())) {
        return json::array();
    }
    TestTracerDir dir;
    auto dir_path_str = dir.path.string();
    const char *argv[] = {xcalbuild_path.c_str(), "-i", dir_path_str.c_str(), "-o", dir_path_str.c_str(), "--profile", "gnu", "-p", "\"pwd\"", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    CommandLine cl;
    cl.parse(argc, (char **)argv);
    ToolchainProfile profile;
    profile.load(cl.get_toolchain_profile());
    profile.load_actionable();
    Tracer tracer(tracing_method, cl, profile);
    tracer.trace();
    fs::ifstream cdb_ifs { dir.path / "compile_commands.json" };

    // Normalise temp file names.
    std::ostringstream sstr;
    sstr << cdb_ifs.rdbuf();
    auto cdb_content = sstr.str();
    boost::regex tmp_exp{"/tmp/[a-zA-Z0-9]+\\."};
    cdb_content = boost::regex_replace(cdb_content, tmp_exp, "tmp.");

    auto cdb = json::parse(cdb_content);
    cdb_ifs.close();

    // Basic tests.
    #ifdef __APPLE__
        BOOST_TEST(cdb.size() == 1);
        BOOST_TEST(ba::ends_with(cdb[0]["arguments"][0].get<std::string>(), "gcc"));
    #else
        BOOST_TEST(cdb.size() == 3);
        BOOST_TEST(ba::ends_with(cdb[0]["arguments"][0].get<std::string>(), "cc1"));
        BOOST_TEST(ba::ends_with(cdb[1]["arguments"][0].get<std::string>(), "as"));
        BOOST_TEST(ba::ends_with(cdb[2]["arguments"][0].get<std::string>(), "ld"));
    #endif

    return cdb;
}

BOOST_AUTO_TEST_CASE(test_bear_and_strace) {
    // Test BEAR
    auto cdb_bear = test_trace(TracingMethod::BEAR);

    // Test STRACE
    if(!bp::search_path("strace").string().length()) {
        return;
    }
    auto cdb_strace = test_trace(TracingMethod::STRACE);

    // Check if they produce the same.
    BOOST_TEST(cdb_bear == cdb_strace, "bear:\n" << cdb_bear.dump(2) << "\nstrace:\n" << cdb_strace.dump(2));
}
