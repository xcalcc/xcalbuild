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
#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include "win-tracer.hpp"

using namespace std;
namespace fs = boost::filesystem;

bool init_unit_test() {
	return true;
}

BOOST_AUTO_TEST_CASE(test_utf16_to_utf8) {
	char* result = utf16_to_utf8(L"Test");
	BOOST_TEST(strcmp("Test", result) == 0);
	result = utf16_to_utf8(L"");
	BOOST_TEST(strcmp("", result) == 0);
}

BOOST_AUTO_TEST_CASE(test_quote_argument) {
	wstring output1;
	quote_argument(wstring(L"no_space"), output1);
	BOOST_TEST(output1.compare(L"no_space") == 0);

	wstring output2;
	quote_argument(wstring(L"with space"), output2);
	BOOST_TEST(output2.compare(L"\"with space\"") == 0);

	wstring output3;
	quote_argument(wstring(L"\\ "), output3);
	BOOST_TEST(output3.compare(L"\"\\ \"") == 0);
}

BOOST_AUTO_TEST_CASE(test_create_arg_string) {
	wchar_t* args[2] = { L"arg 1", L"arg2" };
	wchar_t* output;
	create_arg_str(args, &output, 2);
	BOOST_TEST(wcscmp(output, L"\"arg 1\" arg2") == 0);
}

BOOST_AUTO_TEST_CASE(test_args_from_response_file) {
	ofstream test_resp("./test-resp");
	test_resp << "arg1" << endl;
	test_resp << "arg2 arg3" << endl;
	test_resp << "arg4" << endl;
	test_resp.close();
	int num_args;
	LPWSTR* output = args_from_response_file("./test-resp", &num_args, (char*)".");
	BOOST_TEST(num_args == 4);
	BOOST_TEST(wcscmp(output[0], L"arg1") == 0);
	BOOST_TEST(wcscmp(output[1], L"arg2") == 0);
	BOOST_TEST(wcscmp(output[2], L"arg3") == 0);
	BOOST_TEST(wcscmp(output[3], L"arg4") == 0);
	fs::remove("./test-resp");
}

BOOST_AUTO_TEST_CASE(test_config_reader) {
	ofstream out("./test.json");
	out << "[" << endl;
	out << "{\"binary\":\"test\", \"responseFileArgs\":[{\"argument\":\"-f\", \"argFormat\":[\"space\"]}]}," << endl;
	out << "{\"binary\":\"test2\", \"responseFileArgs\":[{\"argument\":\"@\", \"argFormat\":[\"attached\"]}]}" << endl;
	out << "]" << endl;
	out.close();
	CompilerConfig config(L"./test.json");
	BOOST_TEST(config.is_interesting_binary("test"));
	BOOST_TEST(!config.is_interesting_binary("invalid"));
	auto argument = config.is_response_file_arg("test", "-f");
	BOOST_TEST(argument.has_value() == true);
	vector<string> args = argument->second;
	bool res = find(args.begin(), args.end(), "space") != args.end();
	BOOST_TEST(res == true);
	auto argument2 = config.is_response_file_arg("test2", "@./foo");
	vector<string> args2 = argument2->second;
	bool res2 = find(args2.begin(), args2.end(), "attached") != args2.end();
	BOOST_TEST(res2 == true);
	fs::remove("./test.json");
}