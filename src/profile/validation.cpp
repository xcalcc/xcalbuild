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

#include "validation.hpp"
#include "logger.hpp"

using namespace xcal;

void xcal::validate_json(
    const nlohmann::json &json,
    const std::string &key,
    JSONType type,
    bool lookup_key,
    bool print_json,
    bool allow_empty) noexcept(false)
{
    if(lookup_key && !json.count(key)) {
        if(print_json) {
            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "In " << json.dump() << ", property '" << key << "' is missing";
        } else {
            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Property '" << key << "' is missing";
        }
        throw IncorrectProfileException();
    }

    const nlohmann::json &j = lookup_key ? json[key] : json;

    bool valid = true;
    std::string ty = "an array";
    switch(type) {
        case JT_ARRAY:
            valid = j.is_array() && (allow_empty || (j.size() > 0));
            ty = std::string(allow_empty ? "an" : "a nonempty" ) + " array";
            break;
        case JT_STRING:
            valid = j.is_string() && (allow_empty || (j.get<std::string>().length() > 0));
            ty = std::string(allow_empty ? "a" : "a nonempty" ) + " string";
            break;
        case JT_STRING_ARRAY:
            valid = j.is_string() && (allow_empty || (j.get<std::string>().length() > 0));
            ty = "an array of strings";
            break;
        case JT_OBJECT:
            valid = j.is_object() && (allow_empty || (j.size() > 0));
            ty = std::string(allow_empty ? "an" : "a nonempty" ) + " object";
            break;
        case JT_OBJECT_ARRAY:
            valid = j.is_object() && (allow_empty || (j.size() > 0));
            ty = "an array of objects";
            break;
    }

    if(!valid) {
        if(lookup_key && print_json) {
            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "In " << json.dump() << ", property '" << key.c_str() << "' is not " << ty;
        } else {
            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Property '" << key.c_str() << "' is not " << ty;
        }
        throw IncorrectProfileException();
    }
}