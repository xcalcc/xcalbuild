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

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>
#include <boost/optional.hpp>
#include "compiler-config.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>

using namespace std;
using json = nlohmann::json;

CompilerConfig::CompilerConfig(wchar_t* json_path) {
    ifstream file_input(json_path);
    this->original_json = json::parse(file_input);
    file_input.close();
    if (!this->original_json.is_array()) {
        return;
    }
    for (json j : this->original_json) {
        string compiler = j["binary"];
        this->compilers.push_back(compiler);
        vector<ResponseFileEntry> responseFileArgs;
        for (json k : j["responseFileArgs"]) {
            vector<string> format;
            for (string fmt : k["argFormat"]) {
                format.push_back(fmt);
            }
            responseFileArgs.push_back(pair<string, vector<string>>(k["argument"], format));
        }
        this->response_file_arguments.insert(pair<string, vector<ResponseFileEntry>>(compiler, responseFileArgs));
    }
}

bool CompilerConfig::is_interesting_binary(char* name) {
    // If no binary specified, treat all as interesting.
    // For debug usage only.
    if(!this->compilers.size()) {
        return true;
    }
    auto res = find_if(this->compilers.begin(), this->compilers.end(), [&](string compiler) {
        return compiler.compare(name) == 0;
    });
    return res != this->compilers.end();
}

boost::optional<ResponseFileEntry> CompilerConfig::is_response_file_arg(char *exe_name, char* arg) {
    auto entry = this->response_file_arguments.find(string(exe_name));
    if (entry != this->response_file_arguments.end()) {
        vector<ResponseFileEntry> args = entry->second;
        auto res = find_if(args.begin(), args.end(), [&](ResponseFileEntry a) {
            // For attached response file args, check if the given arg has the value as a prefix
            if (find_if(a.second.begin(), a.second.end(),
                [](string& s) {return s.compare("attached") == 0; }) != a.second.end()) {
                return boost::starts_with(arg, a.first);
            }
            // Otherwise it's just a straight string comparison
            return a.first.compare(arg) == 0;
        });
        if (res == args.end()) {
            return boost::none;
        }
        return *res;
    }
    return boost::none;
}
