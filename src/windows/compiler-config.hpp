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
#include <vector>
#include <string>
#include <boost/optional.hpp>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

typedef pair<string, vector<string>> ResponseFileEntry;

class CompilerConfig {
public:
	CompilerConfig(wchar_t* json_path);
	bool is_interesting_binary(char* name);
	boost::optional <ResponseFileEntry> is_response_file_arg(char* exe_name, char* arg);

private:
	json original_json;
	vector<string> compilers;
	map<string, vector<ResponseFileEntry>> response_file_arguments;
};