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

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <processthreadsapi.h>
#include <debugapi.h>
#include <shellapi.h>
#include <winternl.h>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "compiler-config.hpp"

#define STATUS_WX86_BREAKPOINT 0x4000001F

#define RTL_USER_PROCESS_OFFSET_64 0x20

#define CWD_OFFSET_64 0x38

#define CMDLINE_OFFSET_64 0x70

typedef BOOL(WINAPI* IsWow64ProcessPtr)(
    IN HANDLE hProcess,
    OUT PBOOL Wow64Process
);

typedef NTSTATUS(NTAPI* NtQueryInformationProcessPtr)(
    IN HANDLE hProcess,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLenght OPTIONAL
);

using namespace std;

using json = nlohmann::json;

/** 
    Allocates and returns a utf8 string from a utf16 string

    @param wstr utf16 string toc convert
    @returns utf8 string
*/
char* utf16_to_utf8(wchar_t* wstr) {
    int count;
    char* buff;
    count = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    buff = new char[count];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buff, count, NULL, NULL);
    return buff;
}
/**
    Implements argument quoting as per Microsoft's rules.
    Adapted from https://docs.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way

    @param argument The argument to be potentially quoted
    @param quoted_arg The result of quoting. Where quoting is not required, it will be the
                      same value as the argument parameter.
 */
void quote_argument(const wstring argument, wstring& quoted_arg) {
    if ((argument.empty() == false) && argument.find_first_of(L" \t\n\v\"") == argument.npos) {
        quoted_arg.append(argument);
    } else {
        quoted_arg.push_back(L'"');
        for (auto it = argument.begin(); ; ++it) {
            unsigned num_backslashes = 0;
            while (it != argument.end() && *it == L'\\') {
                ++it;
                ++num_backslashes;
            }
            if (it == argument.end()) {
                quoted_arg.append(num_backslashes * 2, L'\\');
                break;
            } else if (*it == L'"') {
                quoted_arg.append(num_backslashes * 2 + 1, L'\\');
                quoted_arg.push_back(*it);
            } else {
                quoted_arg.append(num_backslashes, L'\\');
                quoted_arg.push_back(*it);
            }
        }
        quoted_arg.push_back(L'"');
    }
}

/**
    Creates a command line string suitable to pass to system functions, including quoting arguments
    where required.

    @param original_args the original argv values from the process
    @param cmdline pointer to use when allocating a new command line string. This argument will
                   have new memory assigned to it.
    @param num_args the number of arguments in original_args 
*/
void create_arg_str(wchar_t* original_args[], wchar_t** cmdline, size_t num_args) {
    wstring quoted_str;
    for (size_t i = 0; i < num_args; i++) {
        wstring cur_arg(original_args[i]);
        wstring quoted_arg;
        quote_argument(cur_arg, quoted_arg);
        quoted_str.append(quoted_arg);
        if (i < (num_args - 1)) {
            quoted_str.push_back(L' ');
        }
    }
    *cmdline = new wchar_t[quoted_str.size() + 1];
    wcscpy(*cmdline, quoted_str.c_str());
}

/**
    Opens and reads a response file.

    @param response_file_path path to the response file to read
    @param num_response_file_args the size of the returned array
    @param proc_cwd the cwd of the process that the response file belongs to
    @returns array of arguments from the response file
*/

LPWSTR *args_from_response_file(char* response_file_path, int *num_response_file_args, char* proc_cwd) {
    size_t tracer_cwd_size = GetCurrentDirectory(0, NULL) + 1;
    char* tracer_cwd = new char[tracer_cwd_size];
    GetCurrentDirectory(tracer_cwd_size, tracer_cwd);

    SetCurrentDirectory(proc_cwd);
    
    wstring cmdline;
    wifstream input(response_file_path, ios_base::in);
    vector<wstring> arg_lines;

    // Windows response files can be either a single line
    // or multiple. Newlines should be counted as spaces.
    // This cleans up the input file into a string we can
    // pass to the system to parse for us.
    for (wstring line; getline(input, line);) {
        arg_lines.push_back(line);
    }

    if (arg_lines.size() == 1) {
        cmdline = arg_lines[0];
    } else {
        bool first = true;
        for (wstring str : arg_lines) {
            if (!first) {
                cmdline.append(L" ");
            } else {
                first = false;
            }
            cmdline.append(str);
        }
    }
    

    // Get rid of UTF8 BOM if present
    if (cmdline.compare(0, 3, L"\xEF\xBB\xBF") == 0) {
        cmdline.erase(0, 3);
    }

    SetCurrentDirectory(tracer_cwd);
    delete[] tracer_cwd;
    
    // Should now be in a form like a command line, so use the
    // system library to parse it.
    if (arg_lines.size() == 0) {
        return NULL;
    }

    return CommandLineToArgvW(cmdline.c_str(), num_response_file_args);
}

/**
    Fetches the error message from the error code returned from GetLastError

    @param error_code the code returned from GetLastError
    @returns the error message
*/
wchar_t* error_message_from_code(DWORD error_code) {
    wchar_t* buffer;
    DWORD result = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        0,
        (wchar_t*)&buffer,
        0,
        NULL);
    if (result) {
        return buffer;
    } else {
        return NULL;
    }
}

/**
    A wrapper around the ugliness of calling NTQueryInformationProcess to fetch the param block

    @param process_handle handle to process
    @returns A pointer to the param block for the process specified by the process_handle
*/
PCHAR get_param_block(HANDLE process_handle) {
    NtQueryInformationProcessPtr NtQueryInformationProcess = 
        (NtQueryInformationProcessPtr) GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationProcess"); 
    DWORD ret_status, size_needed;
    PROCESS_BASIC_INFORMATION pbi;
    PCHAR peb, param_block;
    size_t param_block_offset;
    if (!NtQueryInformationProcess) {
        return NULL;
    }

    ret_status = NtQueryInformationProcess(
        process_handle,
        ProcessBasicInformation,
        &pbi,
        sizeof(PROCESS_BASIC_INFORMATION),
        &size_needed
    );

    peb = (PCHAR) pbi.PebBaseAddress;
    param_block_offset = RTL_USER_PROCESS_OFFSET_64;
 

    if (ret_status != 0) {
        return NULL;
    }
    ReadProcessMemory(process_handle, peb + param_block_offset, &param_block, sizeof(param_block), NULL);
    return param_block;
}

/**
    Reads a value out of a param_block using the offset provided

    @param proces_handle handle for the process that owns the param_block
    @param param_block pointer to the param_block returned from get_param_block
    @param offset the offset ot use when reading in the param_block
    @returns the value from the param block at the give offset
*/
wchar_t *read_from_param_block(HANDLE process_handle, PCHAR param_block, size_t offset) {
    size_t length;
    PWCHAR buffer;
    wchar_t* out_buffer;
    size_t out_buffer_size;
   
    UNICODE_STRING str_obj;
    if (!ReadProcessMemory(process_handle, param_block + offset, &str_obj, sizeof(str_obj), NULL)) {
        DWORD error_code = GetLastError();
        wchar_t* message = error_message_from_code(error_code);
        wcerr << "Error getting param_block" << endl;
        if (message) {
            wcerr << message << endl;
        } else {
            wcerr << "Unable to get further error information" << endl;
        }
        return NULL;
    }
    length = str_obj.Length;
    buffer = str_obj.Buffer;
   
    out_buffer_size = ((length / sizeof(wchar_t)) + 1);
    out_buffer = new wchar_t[out_buffer_size];
    ZeroMemory(out_buffer, length + sizeof(wchar_t));
    SetLastError(0);
    if (!ReadProcessMemory(process_handle, (PCHAR)buffer, out_buffer, length, NULL)) {
        DWORD error_code = GetLastError();
        wchar_t* message = error_message_from_code(error_code);
        wcerr << "Error reading buffer from param_block" << endl;
        if (message) {
            wcerr << message << endl;
        } else {
            wcerr << "Unable to get further error information" << endl;
        }
        return NULL;
    }
    return out_buffer;
}

/**
    Reads all the information about a process and creates a json entry for the compiler database.

    @param process_handle the handle to the process
    @param cdb_obj the json object created by this function
    @param config the compiler config to use to parse response file args and filter binary names
    @returns true if the cdb_obj was updated (i.e. we want to log the process)
*/
bool log_create_process(HANDLE process_handle, json &cdb_obj, CompilerConfig &config) { 
    PCHAR param_block = get_param_block(process_handle);
    wchar_t* cwd;
    wchar_t* cmdline;
    LPWSTR* argv;
    int num_args;

    if (!param_block) {
        wcerr << "Cannot read parameter block for process" << endl;
        return false;
    }
    cwd = read_from_param_block(
        process_handle,
        param_block,
        CWD_OFFSET_64
        );

    if (!cwd) {
        cerr << "Cannot read cwd from process_block" << endl;
        return false;
    }

    cmdline = read_from_param_block(
        process_handle,
        param_block,
        CMDLINE_OFFSET_64
        );

    if (!cmdline) {
        cerr << "Cannot read cmdline from process_block" << endl;
        return false;
    }


    char* cwd_utf8 = utf16_to_utf8(cwd);
    argv = CommandLineToArgvW(cmdline, &num_args);
    json arguments =  json::array({});

    if (num_args == 0) {
        return false;
    }

    char* exe_path = utf16_to_utf8(argv[0]);
    char exe_name[MAX_PATH];

    // Gets the executable name
    _splitpath_s(exe_path, NULL, 0, NULL, 0, exe_name, MAX_PATH, NULL, 0);

    // Windows file system is case-insensitive, so compare as lower case
    boost::to_lower(exe_name);

    // Check if we actually want to log this process or not
    if (!config.is_interesting_binary(exe_name)) {
        return false;
    }

    // Process the arguments of the executable
    for (int i = 0; i < num_args; i++) {
        char* utf8_arg = utf16_to_utf8(argv[i]);
        auto response_arg_type = config.is_response_file_arg(exe_name, utf8_arg);
        // If it's not a response file, just add it to the json object and continue
        if (!response_arg_type.has_value()) {
            if (!boost::filesystem::windows_name(utf8_arg)) {
                // Some compilers (iccarm/iarbuild) use some funny path conventions for relative paths
                // this overcomes that
                boost::filesystem::path p = boost::filesystem::weakly_canonical(utf8_arg);
                arguments.push_back(json::string_t(p.string()));
            }
            else {
                arguments.push_back(json::string_t(utf8_arg));
            }
            continue;
        }

        // Have a response file arg, we need to process the respones file contents
        char* response_path = NULL;
        vector<string> split_str;
        string arg_str(utf8_arg);
      
        // Use the split string later to determine if the argument type is equals
        boost::algorithm::split(split_str, arg_str, boost::is_any_of("="));
        for (string &kind : response_arg_type->second) {
            if (kind.compare("attached") == 0) {
                response_path = utf8_arg + response_arg_type->first.size(); 
                // If it's attached, we don't support also being "equal" or "space"
                // so break.
                break;
            }

            if (kind.compare("equal") == 0 && split_str.size() > 1) {
                response_path = (char *)split_str[1].c_str();
                break;
            }

            if (kind.compare("space") == 0 && split_str.size() == 1) {
                if (++i >= num_args) {
                    cerr << "Error parsing response file argument" << endl;
                    break;
                }
                response_path = utf16_to_utf8(argv[i]);
            }
        }

        if (response_path == NULL) {
            break;
        }

        // Read and insert the contents of the response file into the json object. Order could be important
        // so this must be done at this point.
        int num_response_file_args = 0;
        LPWSTR *response_file_args = args_from_response_file(response_path, &num_response_file_args, cwd_utf8);
        if (response_file_args != NULL) {
            for (int j = 0; j < num_response_file_args; j++) {
                char* utf8_arg = utf16_to_utf8(response_file_args[j]);
                // As above, handle odd paths generated by iarbuild/iccarm
                if (!boost::filesystem::windows_name(utf8_arg)) {
                    boost::filesystem::path p = boost::filesystem::weakly_canonical(utf8_arg);
                    arguments.push_back(json::string_t(p.string()));
                } else {
                    arguments.push_back(json::string_t(utf8_arg));
                }
                delete[] utf8_arg;

            }
        } 
        delete[] utf8_arg;
    }
    delete[] cwd;
    delete[] cmdline;
    delete[] exe_path;

    cdb_obj["directory"] = json::string_t(cwd_utf8);
    cdb_obj["arguments"] = arguments;

    return true;
}

/**
    The main loop to watch and trace processes.

    @param exec_path the path of the executable
    @param exec_args the arguments to the executable
    @param arg_size size of exec_args
    @param out_file the path to the file to output the json compiler database
    @param config the compiler configs used to determine if the binary needs to be logged and the response file args
    @returns the exit code of the process being traced
*/
int trace_process(wchar_t* exec_path, wchar_t* exec_args[], const size_t arg_size, const wchar_t* out_file, CompilerConfig &config) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    HANDLE h;
    DWORD root_process_exit_status = EXIT_FAILURE;
    int process_flags;
    wchar_t* cmdline;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    process_flags = CREATE_SUSPENDED | DEBUG_PROCESS;

    create_arg_str(exec_args, &cmdline, arg_size);

    SetLastError(0);
    BOOL ret = CreateProcessW(exec_path,
        cmdline,
        NULL,
        NULL,
        TRUE,
        process_flags,
        NULL,
        NULL,
        &si,
        &pi);

    if (!ret) {
        DWORD err_code = GetLastError();
        wcerr << "Error occured while creating process" << endl;
        wchar_t* err_message = error_message_from_code(err_code);
        if (err_message) {
            wcerr << err_message << endl;
        } else {
            wcerr << "Unable to get further error information" << endl;
        }
        return EXIT_FAILURE;
    }

    // Start process
    ResumeThread(pi.hThread);
    BOOL process_running = true;
    EXCEPTION_RECORD exception_rec;
    json process_obj;
    json array = json::array();

    while (process_running) {
        DEBUG_EVENT event;
        json process_obj = json();
        DWORD continue_status = DBG_CONTINUE;
        if (!WaitForDebugEvent(&event, INFINITE)) {
            break;
        }

        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, event.dwProcessId);
            if (h == NULL) {
                wcerr << "Error inspecting process" << endl;
                break;
            }
            if (log_create_process(h, process_obj, config)) {
                array.push_back(process_obj);
            }
            CloseHandle(h);
        }

        // For a exit process event, if the process exiting is the
        // parent process we exit the loop.
        if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT
            && event.dwProcessId == pi.dwProcessId) {
            process_running = false;
            root_process_exit_status = event.u.ExitProcess.dwExitCode;
        }


        if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            exception_rec = event.u.Exception.ExceptionRecord;
            if (exception_rec.ExceptionCode != EXCEPTION_BREAKPOINT &&
                exception_rec.ExceptionCode != STATUS_WX86_BREAKPOINT) {
                // We didn't handle the exception, so pass it up
                // Avoids infinite loops of exceptions being thrown
                continue_status = DBG_EXCEPTION_NOT_HANDLED;
            }
        }

        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continue_status);
    }
    delete[] cmdline;

    ofstream out_stream (out_file);
    out_stream << array.dump(4) << endl;
    out_stream.close();

    return root_process_exit_status;
}

