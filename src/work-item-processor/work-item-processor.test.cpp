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

#include <nlohmann/json.hpp>

#include "work-item-processor.hpp"
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
    auto log_file = fs::system_complete(fs::path("work-item-processor-test.log")).string();
    std::string trace_id = "TRACEID";
    std::string span_id = "SPANID";
    init_logger(log_file, trace_id, span_id, /*debug*/false, /*local_log*/true);
    return true;
}

BOOST_AUTO_TEST_CASE( test_corner_cases ) {
    ToolchainProfile profile;
    const char *argv[] = {"xcalbuild", "-i", ".", "-o", ".", "--", "make"};
    int argc = sizeof(argv) / sizeof(const char *);
    CommandLine cl;
    cl.parse(argc, (char **)argv);
    BuildProcessor bp(profile, cl, /*parallelism*/1);
    auto work_item = R"({
        "directory": "/work",
        "arguments": ["gcc", "@tmp", "-k", "-p"]
    })"_json;
    WorkItemProcessor wip(profile, cl, work_item, &bp);

    ParsedWorkItem parsed;

    // Empty link target.
    parsed.target = "";
    parsed.kind = CommandKind::CK_LINK;
    wip.handle_link(parsed);
    BOOST_TEST(parsed.target == "DEFAULT_OUTPUT");
    parsed.kind = CommandKind::CK_ARCHIVE;
    parsed.target = "";
    wip.handle_link(parsed);
    BOOST_TEST(parsed.target == "");

    // Transient source file.
    parsed.sources.push_back(std::make_pair("", FileFormat::FF_C_SOURCE));
    parsed.kind = CommandKind::CK_COMPILE;
    wip.handle_compile(nullptr, parsed);
}
