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

#include "checksum.hpp"
#include "profiler.hpp"
#include "logger.hpp"

using namespace xcal;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_CASE( test_checksum )
{
    Checksum cs("");
    BOOST_TEST(cs.output() == "da39a3ee5e6b4b0d3255bfef95601890afd80709");

    cs.process("abc");
    BOOST_TEST(cs.output() == "cdc4c98e1308bbcdc027324511ac6156003b3dc1");
}

bool init_unit_test() {
    auto log_file = fs::system_complete(fs::path("utils-test.log")).string();
    std::string trace_id = "TRACEID";
    std::string span_id = "SPANID";
    init_logger(log_file, trace_id, span_id, /*debug*/false, /*local_log*/true);
    return true;
}