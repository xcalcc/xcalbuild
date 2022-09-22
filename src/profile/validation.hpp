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

#include <boost/optional.hpp>
#include <nlohmann/json.hpp>

#include "error-code.hpp"
#include "logger.hpp"

namespace xcal {

/**
 * @brief Error parsing/typechecking profile.
 *
 */
struct IncorrectProfileException : public std::exception {
	const char *what() const noexcept {
    	return "Incorrect profile";
    }
    const ErrorCode error_code = EC_INCORRECT_TOOLCHAIN_PROFILE;
};

enum JSONType {
    JT_STRING,
    JT_OBJECT,
    JT_ARRAY,
    JT_STRING_ARRAY,
    JT_OBJECT_ARRAY,
};

/**
 * @brief Validate the json against a type and other specs.
 * Throws IncorrectProfileException exception if failed to validate.
 *
 * @param json the json to be validated
 * @param key the property key into the json object
 * @param type the expected type
 * @param lookup_key if true, validate json[key], otherwise validate json
 * @param print_json if true, print json in the log message
 * @param allow_empty if true, allow empty type, otherwise forbids empty string/array/object
 */
void validate_json(
    const nlohmann::json &json,
    const std::string &key,
    JSONType type,
    bool lookup_key = true,
    bool print_json = true,
    bool allow_empty = false
) noexcept(false);

/**
 * @brief Check whether the optional value is none.
 * Throws IncorrectProfileException exception if none.
 *
 * @tparam T
 * @param val the value to be checked
 * @param key the property key into the json object
 * @param json the json object to be printed in the log message
 */
template<typename T>
void validate_optional(
    const boost::optional<T> &val,
    const std::string &key,
    const nlohmann::json &json = nlohmann::json()
) noexcept(false) {
    if(!val) {
        if(json.count(key)) {
            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "In " << json.dump() << ", property '" << key << "' is invalid";
        } else {
            LOG_EC(error, EC_INCORRECT_TOOLCHAIN_PROFILE) << "Property '" << key << "' is invalid";
        }
        throw IncorrectProfileException();
    }
}

}
