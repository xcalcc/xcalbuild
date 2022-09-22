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
#include "compiler-config.hpp"

char* utf16_to_utf8(wchar_t* wstr);
void quote_argument(const wstring argument, wstring& quoted_arg);
void create_arg_str(wchar_t* original_args[], wchar_t** cmdline, size_t num_args);
LPWSTR* args_from_response_file(char* response_file_path, int* num_response_file_args, char* proc_cwd);
wchar_t* error_message_from_code(DWORD error_code);

int trace_process(wchar_t* exec_path, wchar_t* exec_args[], const size_t arg_size, const wchar_t* out_file, CompilerConfig& config);