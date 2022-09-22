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
#include <boost/algorithm/string.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/optional.hpp>
#include <boost/process.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include <nlohmann/json.hpp>

#include "build-processor.hpp"
#include "command-line.hpp"
#include "toolchain-profile.hpp"
#include "logger.hpp"
#include "error-code.hpp"

using namespace xcal;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
using json = nlohmann::json;

bool init_unit_test() {
    auto log_file = fs::system_complete(fs::path("build-processor-test.log")).string();
    std::string trace_id = "TRACEID";
    std::string span_id = "SPANID";
    init_logger(log_file, trace_id, span_id, /*debug*/false, /*local_log*/true);
    return true;
}

fs::path touch_temp(const fs::path &output_path) {
    auto temp_path = output_path / fs::unique_path();
    fs::ofstream ofs{ temp_path };
    ofs.close();
    return temp_path;
}

BOOST_AUTO_TEST_CASE( test_bad_cdb ) {
    ToolchainProfile profile;
    std::string output_dir = "build-processor-test-tmp";
    auto output_path = fs::system_complete(fs::path(output_dir));
    fs::remove_all(fs::path(output_dir));
    fs::create_directories(fs::path(output_dir));

    // Bad CDB.
    fs::ofstream cdb{ output_path / fs::path("compile_commands.json") };
    cdb << "[-]" << std::endl;
    cdb.close();

    const char *argv[] = {"xcalbuild", "-i", ".", "-o", "build-processor-test-tmp", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    CommandLine cl;
    cl.parse(argc, (char **)argv);
    BuildProcessor bp(profile, cl, /*parallelism*/1);

    BOOST_CHECK(bp.process() == EC_ERROR_PARSING_CDB);

    fs::remove_all(fs::path(output_dir));
}

BOOST_AUTO_TEST_CASE( test_generate_output ) {
    ToolchainProfile profile;
    std::string output_dir = "build-processor-test-tmp";
    auto output_path = fs::system_complete(fs::path(output_dir));
    fs::remove_all(output_path);
    fs::create_directories(output_path);

    // CDB, have link entries to test WorkItemProcessor invocation.
    // Now that WorkItemProcessor depends on tool profile, the correctness
    // will be tested elsewhere.
    fs::ofstream cdb{ output_path / fs::path("compile_commands.json") };
    cdb << R"([{
        "directory": "/work/lib",
        "arguments": ["/usr/bin/ar", "r", "lib.a", "src1.c.o", "src2.c.o"]
    },{
        "directory": "/work/exe",
        "arguments": ["/usr/bin/ld", "-o", "exe", "lib.a", "src1.c.o"]
    },{
        "directory": "/work/exe",
        "arguments": ["/usr/bin/ld", "-o", "test-exe", "src1.c.o"]
    }])" << std::endl;
    cdb.close();

    const char *argv[] = {"xcalbuild", "-i", ".", "-o", "build-processor-test-tmp", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    CommandLine cl;
    cl.parse(argc, (char **)argv);
    BuildProcessor bp(profile, cl, /*parallelism*/1);

    const auto append_cc_result = [&](
        const char *target,
        const char *file,
        FileFormat format,
        const char *filename,
        std::vector<std::string> c_scan_options,
        std::vector<std::string> cxx_scan_options,
        std::set<std::string> files
    ) {
        ResultFile res_file(
            touch_temp(output_path),
            "da39a3ee5e6b4b0d3255bfef95601890afd80709");
        CCResult res(
            std::move(res_file),
            target,
            file,
            format,
            filename,
            std::move(c_scan_options),
            std::move(cxx_scan_options),
            std::move(files));
        bp.append_cc_result(std::move(res));
    };

    const auto append_ld_result = [&](
        const char *target,
        std::vector<std::string> &&sources
    ) {
        LDResult res(
            target,
            std::move(sources));
        bp.append_ld_result(std::move(res));
    };

    const auto append_as_result = [&](
        const char *target,
        const char *source
    ) {
        ASResult res(target, source);
        bp.append_as_result(std::move(res));
    };

    // gcc -c -o -ansi /work/lib1/src1.cc.o /work/lib1/src1.cc
    std::vector<std::string> c_scan_options = {"-ansi"};
    std::vector<std::string> cxx_scan_options = {"-ansi"};
    std::set<std::string> deps;
    deps.insert("/work/include/inc1.h");
    append_cc_result(
        "/work/lib1/src1.s",
        "/work/lib1/src1.cc",
        FileFormat::FF_CXX_SOURCE,
        "src1.cc.ii",
        c_scan_options,
        cxx_scan_options,
        deps
    );
    append_as_result("/work/lib1/src1.cc.o", "/work/lib1/src1.s");

    // gcc -c -o /work/lib1/src2.c.o /work/lib1/src2.c
    deps.clear();
    deps.insert("/work/include/inc2.h");
    append_cc_result(
        "/work/lib1/src2.s",
        "/work/lib1/src2.c",
        FileFormat::FF_C_SOURCE,
        "src2.c.i",
        c_scan_options,
        cxx_scan_options,
        deps
    );
    append_as_result("/work/lib1/src2.c.o", "/work/lib1/src2.s");

    std::vector<std::string> sources = {"/work/lib1/src1.cc.o", "/work/lib1/src2.c.o"};
    append_ld_result("/work/lib1/lib.a", std::move(sources));

    // gcc -c -o -ansi /work/lib2/src1.cc.o /work/lib2/src1.cc
    deps.clear();
    deps.insert("/work/include/inc1.h");
    append_cc_result(
        "/work/lib2/src1.s",
        "/work/lib2/src1.cc",
        FileFormat::FF_CXX_SOURCE,
        "src1.cc.ii",
        c_scan_options,
        cxx_scan_options,
        deps
    );
    append_as_result("/work/lib2/src1.cc.o", "/work/lib2/src1.s");

    sources = {"/work/lib2/src1.cc.o"};
    append_ld_result("/work/lib2/lib.a", std::move(sources));

    // gcc -c -o /work/exe/src1.cc.o /work/exe/src1.cc
    deps.clear();
    deps.insert("/work/include/inc1.h");
    append_cc_result(
        "/work/exe/src1.s",
        "/work/exe/src1.cc",
        FileFormat::FF_CXX_SOURCE,
        "src1.cc.ii",
        c_scan_options,
        cxx_scan_options,
        deps
    );
    append_as_result("/work/exe/src1.cc.o", "/work/exe/src1.s");

    sources = {"/work/exe/src1.cc.o", "/work/lib1/lib.a", "/work/lib2/lib.a"};
    append_ld_result("/work/exe/exe", std::move(sources));

    sources = {"/work/exe/src1.cc.o", "/work/lib1/src1.cc.o", "/work/lib2/src1.cc.o"};
    append_ld_result("/work/exe/test-exe", std::move(sources));

    // This target is not linked.
    // gcc -c -o /work/exe/src2.c.o /work/exe/src2.c
    deps.clear();
    deps.insert("/work/include/inc2.h");
    append_cc_result(
        "/work/exe/src2.s",
        "/work/exe/src2.c",
        FileFormat::FF_C_SOURCE,
        "src2.c.i",
        c_scan_options,
        cxx_scan_options,
        deps
    );
    append_as_result("/work/exe/src2.c.o", "/work/exe/src2.s");

    bp.process();

    auto pp_dir = output_path / "preprocess";
    fs::create_directories(pp_dir);
    boost::process::child pp(
        boost::process::search_path("tar"),
        "-C", pp_dir, "-xzf", output_path / "preprocess.tar.gz");
    pp.wait(); //wait for the process to exit
    BOOST_TEST(pp.exit_code() == 0);

    BOOST_TEST(fs::exists(output_path / "source_files.json"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "checksum.sha1"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "exe.dir" / "preprocess" / "src1.cc.ii"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "exe.dir" / "xcalibyte.properties"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "lib.a.dir" / "preprocess" / "src1.cc.ii"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "lib.a.dir" / "preprocess" / "src2.c.i"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "lib.a.dir" / "xcalibyte.properties"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "lib.a.1.dir" / "preprocess" / "src1.cc.ii"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "lib.a.1.dir" / "xcalibyte.properties"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "test-exe.dir" / "preprocess" / "src1.cc.ii"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "test-exe.dir" / "preprocess" / "src1.cc.1.ii"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "test-exe.dir" / "preprocess" / "src1.cc.2.ii"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "test-exe.dir" / "xcalibyte.properties"));
    BOOST_TEST(fs::exists(output_path / "preprocess" / "xcalibyte.properties"));

    fs::ifstream source_files_ifs{ output_path / "source_files.json" };
    BOOST_TEST(json::parse(source_files_ifs) ==
        R"([
            "/work/exe/src1.cc",
            "/work/include/inc1.h",
            "/work/include/inc2.h",
            "/work/lib1/src1.cc",
            "/work/lib1/src2.c",
            "/work/lib2/src1.cc"
        ])"_json);
    source_files_ifs.close();

    const char *expected_checksum[] = {
        "da39a3ee5e6b4b0d3255bfef95601890afd80709 %/preprocess/test-exe.dir/preprocess/src1.cc.ii",
         "da39a3ee5e6b4b0d3255bfef95601890afd80709 %/preprocess/lib.a.dir/preprocess/src1.cc.ii",
        "da39a3ee5e6b4b0d3255bfef95601890afd80709 %/preprocess/lib.a.dir/preprocess/src2.c.i",
        "da39a3ee5e6b4b0d3255bfef95601890afd80709 %/preprocess/test-exe.dir/preprocess/src1.cc.1.ii",
        "da39a3ee5e6b4b0d3255bfef95601890afd80709 %/preprocess/lib.a.1.dir/preprocess/src1.cc.ii",
        "da39a3ee5e6b4b0d3255bfef95601890afd80709 %/preprocess/test-exe.dir/preprocess/src1.cc.2.ii",
        "da39a3ee5e6b4b0d3255bfef95601890afd80709 %/preprocess/exe.dir/preprocess/src1.cc.ii",
        "a1e0aad31d2ecc6320d5f0e95dcbfd2b6d2ada46 %/preprocess/lib.a.dir/xcalibyte.properties",
        "115781641be3007ae9e5e63d58b3ae311a5e13be %/preprocess/lib.a.1.dir/xcalibyte.properties",
        "afc42df7b80379a6a40e6282731d8d8c5b99acbd %/preprocess/exe.dir/xcalibyte.properties",
        "115781641be3007ae9e5e63d58b3ae311a5e13be %/preprocess/test-exe.dir/xcalibyte.properties",
    };
    auto output_path_str = output_path.string();
    fs::ifstream checksum_ifs{ output_path / "preprocess" / "checksum.sha1" };
    int i = 0;
    for(std::string line; std::getline(checksum_ifs, line); ++i) {
        line = ba::replace_first_copy(line, output_path_str, "%");
        BOOST_TEST(line == std::string{expected_checksum[i]});
    }
    checksum_ifs.close();

    // fs::remove_all(fs::path(output_dir));
}
