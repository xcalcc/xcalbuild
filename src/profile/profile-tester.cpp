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
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/utils/setcolor.hpp>
#include <boost/test/unit_test_log_formatter.hpp>
#include <boost/test/output/compiler_log_formatter.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include "toolchain-profile.hpp"
#include "tool-profile.hpp"
#include "work-item-processor.hpp"
#include "logger.hpp"
#include "config.h"
#include "error-code.hpp"

namespace po = boost::program_options;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;

using namespace boost::unit_test;

using json = nlohmann::json;

using namespace xcal;

/// Log formatter without assertion location.
class LogFormatter : public boost::unit_test::output::compiler_log_formatter {
public:
    void print_prefix(std::ostream &, const_string, std::size_t) override {}
};

// This can't be local as it will be used by other test functions.
ToolProfile profile;

bool parse_command_line(int argc, char *argv[], std::string &profile, std::string &tests, bool &check)
{
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("profile,p",
                po::value<std::string>(&profile)->required(),
                "path to the tool profile")
            ("tests,t",
                po::value<std::string>(&tests),
                "test input JSON file")
            ("check,c",
                "validate the tool profile")
            ("version,v",
                "display version")
            ("help,h",
                "display help message");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "Usage:\n";
            std::cout << "profile-test [options]\n";
            std::cout << desc << "\n";
            exit(EXIT_SUCCESS);
        } else if (vm.count("version")) {
            std::cout << "profile-tester ("
                      << std::string{PROJECT_BUILD_ARCH} << "-"
                      << std::string{PROJECT_BUILD_PLATFORM} << ") "
                      << std::string{PROJECT_VERSION}
                      << " [" << std::string{PROJECT_SOURCE_VERSION}.substr(0,7) << "]\n";
            exit(EXIT_SUCCESS);
        }

        check = vm.count("check");

        if (!check && !vm.count("tests")) {
            std::cerr << "Missing tests file.\n";
            exit(EXIT_FAILURE);
        }

        po::notify(vm);
    } catch (std::exception &e) {
        std::cerr << e.what() << "\n";
        return false;
    // GCOVR_EXCL_START
    } catch (...) {
        std::cerr << "Unknown error.\n";
        return false;
    }
    // GCOVR_EXCL_STOP
    return true;
}

void test_parse(const json &test) {
    ParsedWorkItem parsed;
    profile.parse_work_item(test["command"], parsed);
    json actual = parsed.to_json();

    // Filter actual by expected keys.
    for(const auto &p: test["expected"].items()) {
        auto key = p.key();
        BOOST_TEST(actual.count(key));

        auto a = actual[key];
        auto e = p.value();
        BOOST_TEST(a == e,
            "\nunexpected parse result for '" << key <<
            "'\n===actual:\n" << a.dump(2) <<
            "\n===expected:\n" << e.dump(2));
    }
}

void test_preprocessing_options(const json &test) {
    ParsedWorkItem parsed;
    profile.parse_work_item(test["command"], parsed);
    auto ff = file_format_from_string(test["format"].get<std::string>());
    BOOST_TEST(ff.has_value(), "Invalid file format " << test["format"]);
    auto a = json(
        profile.get_preprocessing_options(
            test["target"].get<std::string>(),
            *ff,
            parsed
        ));
    auto e = test["expected"];
    BOOST_TEST(a.size() == e.size(),
        "\npp option size mismatched" <<
        "'\n===actual:\n" << a.dump(2) <<
        "\n===expected:\n" << e.dump(2));
    // There could be local path in pp options. Just test if e is postfix of a.
    auto eit = e.begin();
    for(auto it = a.begin(); it < a.end(); ++it) {
        BOOST_TEST(ba::ends_with(it->get<std::string>(), eit->get<std::string>()),
            "\npp option mismatch" <<
            "\n===actual:\n" << a.dump(2) <<
            "\n===expected:\n" << e.dump(2));
        ++eit;
    }
}

void test_source_transfromation(const json &test) {
    auto actual = profile.process_source_code(test["source"]);
    auto expected = test["expected"].get<std::string>();
    BOOST_TEST(actual == expected, "unexpected transformation result, actual:\n" << actual << "\nexpected:\n" << expected);
}

bool init_unit_test() {
    std::string profile_path, tests_path;
    bool check = false;
    // Parse command line.
    if(!parse_command_line(
        framework::master_test_suite().argc,
        framework::master_test_suite().argv,
        profile_path, tests_path, check)) {
        std::cerr << "Command line error" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(!fs::exists(profile_path)) {
        std::cerr << "Profile not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    auto log_file = fs::system_complete(fs::path("profile-test.log")).string();

    // Put log to console if validation.
    std::string trace_id = "TRACEIDLOCAL";
    std::string span_id = "SPANID";
    init_logger(log_file, trace_id, span_id, /*debug*/false, /*local_log*/check);

    std::string binary{ framework::master_test_suite().argv[0] };

    boost::unit_test::unit_test_log.set_formatter(new LogFormatter());

    // Load profile as specified.
    auto ec_opt = profile.load(fs::system_complete(profile_path));

    utils::setcolor::state state = 0;
    if(ec_opt) {
        if(check){
            std::cout << utils::setcolor(true, utils::term_attr::BRIGHT, utils::term_color::RED, utils::term_color::ORIGINAL, &state)
                      << "Profile " << fs::system_complete(profile_path) << " is invalid.\n" << utils::setcolor(true, &state);
        }
        exit(boost::exit_test_failure);
    } else if(check) {
        std::cout << utils::setcolor(true, utils::term_attr::BRIGHT, utils::term_color::GREEN, utils::term_color::ORIGINAL, &state)
                  << "Profile " << fs::system_complete(profile_path) << " is valid.\n" << utils::setcolor(true, &state);
        exit(EXIT_SUCCESS);
    }

    profile.load_actionable();

    if(!fs::exists(tests_path)) {
        std::cerr << "Tests not found" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Load and parse the tests.
    fs::ifstream ifs{ fs::path(tests_path) };
    auto tests_json = json::parse(ifs);
    ifs.close();

    // TODO: check version compatibility.
    (void) tests_json["version"];

    for(const auto &test: tests_json["commandlineParsingTests"]) {
        framework::master_test_suite().
            add(BOOST_TEST_CASE_NAME(
                std::bind(&test_parse, test),
                std::string("commandlineParsing ") + test["name"].get<std::string>().c_str()));
    }
    for(const auto &test: tests_json["preprocessingOptionTests"]) {
        framework::master_test_suite().
            add(BOOST_TEST_CASE_NAME(
                std::bind(&test_preprocessing_options, test),
                std::string("preprocessingOption") + test["name"].get<std::string>().c_str()));
    }
    for(const auto &test: tests_json["sourceTransformationTests"]) {
        framework::master_test_suite().
            add(BOOST_TEST_CASE_NAME(
                std::bind(&test_source_transfromation, test),
                std::string("sourceTransformation ") + test["name"].get<std::string>().c_str()));
    }
    return true;
}
