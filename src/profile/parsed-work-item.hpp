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

#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <nlohmann/json.hpp>

#include "fwd.hpp"

namespace xcal {

/**
 * @brief The kind of the command.
 * AS is required to built up the source-target link between ld and cc1.
 * Example: `gcc -o a.out a.c` -> `cc1 -o tmp.s a.c`; `as -o a.o tmp.s`; `ld -o a.out a.o`
 * For link target a.out, the source is actually tmp.s instead of a.o.
 */
enum CommandKind {
    CK_COMPILE = 0,
    CK_ASSEMBLE = 1,
    CK_LINK = 2,
    CK_ARCHIVE = 3,
    CK_IGNORE = 4,
};

boost::optional<CommandKind> command_kind_from_json(const nlohmann::json &json);
std::string command_kind_to_string(CommandKind kind);

boost::optional<FileFormat> file_format_from_string(const std::string &s);
std::string file_format_to_string(FileFormat s);

/**
 * @brief Result from parsing an work item (compile database entry).
 *
 */
struct ParsedWorkItem {
    CommandKind kind = CommandKind::CK_IGNORE;

    /// The binary.
    std::string binary = "";

    /// The CWD of the command.
    std::string dir = "";

    /// The source language specified on the command line.
    FileFormat file_format = FileFormat::FF_EXT;

    /// The sources of the command and their file format, e.g. c source file for compiler.
    std::vector<std::pair<std::string, FileFormat>> sources;

    /// The target of the command, e.g. link target for linker.
    std::string target = "";

    /// The options used in preprocessing.
    std::vector<std::string> pp_options;

    /// The C options passed to xvsa scan.
    std::vector<std::string> c_scan_options;

    /// The C++ options passed to xvsa scan.
    std::vector<std::string> cxx_scan_options;

    /**
     * @brief serialise as a json object.
     *
     * @return nlohmann::json the json representation of the object.
     */
    nlohmann::json to_json();
};

}
