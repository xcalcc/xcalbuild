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
#include <sstream>

#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/optional.hpp>

#include <nlohmann/json.hpp>

#include "prober.hpp"
#include "toolchain-profile.hpp"
#include "logger.hpp"
#include "path.hpp"
#include "error-code.hpp"

using namespace xcal;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
using json = nlohmann::json;

bool init_unit_test() {
    auto log_file = fs::system_complete(fs::path("prober-test.log")).string();
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

BOOST_AUTO_TEST_CASE( test_macro_probe ) {
    Prober prober;
    auto macros = R"({
        "__STDC_VERSION__": {
            "199409L": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}],
            "199901L": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu99"]}]
        },
        "__STDC__": {
            "1": [{"config": "cPrependScanOptions", "action": "prepend", "value": ["-x", "c"]}]
        }
    })"_json;
    auto temp_path = get_temp_path();
    prober.generate_macros_test_file(temp_path, macros);
    fs::ifstream ifs{ temp_path };
    std::ostringstream test_input;
    test_input << ifs.rdbuf();
    ifs.close();
    auto expected = "__XCAL____STDC_VERSION__ __STDC_VERSION__\n"
                    "__XCAL____STDC__ __STDC__\n";
    BOOST_TEST(test_input.str() == expected);

    fs::ofstream ofs{ temp_path};
    ofs << "__XCAL____STDC_VERSION__ 199409L\n"
           "__XCAL____STDC_VERSION__ 201102L\n"
           "__XCAL____STDC__ 1\n"
           "__XCAL____STDCXX__ 1\n"
           "__XCAL____STDC__ __STDC__\n";
    ofs.close();
    auto parsed = prober.parse_macro_expansions(temp_path, macros);
    BOOST_TEST(parsed.size() == 2);
    BOOST_TEST(
        parsed[0] == R"([{"config": "cPrependScanOptions", "action": "prepend", "value": ["-std=gnu89"]}])"_json,
        "\nunexpected actions\n" << parsed[0]);
    BOOST_TEST(
        parsed[1] == R"([{"config": "cPrependScanOptions", "action": "prepend", "value": ["-x", "c"]}])"_json,
        "\nunexpected actions\n" << parsed[1]);

    fs::remove_all(temp_path);
}
