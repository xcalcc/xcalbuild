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

#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <boost/filesystem.hpp>
#include "compiler-config.hpp"
#include "win-tracer.hpp"

extern "C"
int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* output_file = L"compile_commands.json";
    wchar_t* config_json_path = NULL;
    bool parsing_error = false;
    int arg;
    for (arg = 1; arg < argc; ++arg) {
        if (wcscmp(argv[arg], L"/o") == 0) {
            if (++arg == argc) {
                parsing_error = true;
            }
            else {
                output_file = argv[arg];
            }
        }
        else if (wcscmp(argv[arg], L"/c") == 0) {
            if (++arg == argc) {
                parsing_error = true;
            }
            else {
                config_json_path = argv[arg];
            }
        }
        else {
            break;
        }
    }
    if (argc < 2 || parsing_error || config_json_path == NULL) {
        cerr << "Invalid command line" << endl;
        return EXIT_FAILURE;
    }

    if (!boost::filesystem::exists(config_json_path)) {
        cerr << "Config JSON file does not exist" << endl;
        return EXIT_FAILURE;
    }

    CompilerConfig config(config_json_path);
    
    if (arg < argc) {
        wchar_t exe_path[MAX_PATH];
        if (SearchPathW(NULL, argv[arg], L".exe", MAX_PATH, exe_path, NULL) == 0) {
            DWORD error_code = GetLastError();
            wchar_t* message = error_message_from_code(error_code);
            cerr << "Cannot find executable " << utf16_to_utf8(argv[arg]) << endl;
            if (message) {
                wcerr << message << endl;
            } else {
                wcerr << "Unable to get further error information" << endl;
            } 
            return EXIT_FAILURE;
        }

        int exit_status = trace_process(exe_path, (argv + arg), argc - arg, output_file, config);
        return exit_status;
    }

    return EXIT_SUCCESS;
}