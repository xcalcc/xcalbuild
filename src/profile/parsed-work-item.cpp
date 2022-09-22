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

#include <string>

#include <boost/optional.hpp>

#include <nlohmann/json.hpp>

#include "parsed-work-item.hpp"

using json = nlohmann::json;
using namespace xcal;

constexpr const char *command_kind_names[] = {
    "compile",
    "assemble",
    "link",
    "archive",
    "ignore"
};
CHECK_NAMES_SIZE(command_kind_names, CommandKind::CK_IGNORE);

boost::optional<CommandKind> xcal::command_kind_from_json(const nlohmann::json &json) {
    auto s = json.get<std::string>();
    for(unsigned int i = 0; i <= (unsigned int)CommandKind::CK_IGNORE; ++i) {
        if(s.compare(command_kind_names[i]) == 0) {
            return (CommandKind)i;
        }
    }
    return boost::none;
}

std::string xcal::command_kind_to_string(CommandKind kind) {
    return command_kind_names[(unsigned int)kind];
}

constexpr const char *file_format_names[] = {
    "c",
    "c++",
    "preprocessed",
    "assembly",
    "object",
    "library",
    "executive",
    "ext",
};
CHECK_NAMES_SIZE(file_format_names, FileFormat::FF_EXT);

boost::optional<FileFormat> xcal::file_format_from_string(const std::string &f) {
    for(unsigned int i = 0; i <= (unsigned int)FileFormat::FF_EXT; ++i) {
        if(f.compare(file_format_names[i]) == 0) {
            return (FileFormat)i;
        }
    }
    return boost::none;
}

std::string xcal::file_format_to_string(FileFormat f) {
    return file_format_names[(unsigned int)f];
}

json ParsedWorkItem::to_json() {
    json res;
    res["kind"] = command_kind_to_string(kind);
    res["binary"] = binary;
    res["dir"] = dir;
    res["target"] = target;
    res["fileFormat"] = file_format_to_string(file_format);
    res["sources"] = json::array();
    for(const auto &source: sources) {
        json s;
        s["file"] = source.first;
        s["format"] = file_format_to_string(source.second);
        res["sources"].push_back(std::move(s));
    }
    res["ppOptions"] = json(pp_options);
    res["cScanOptions"] = json(c_scan_options);
    res["cxxScanOptions"] = json(cxx_scan_options);
    return res;
}