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

#include <iostream>
#include <boost/uuid/detail/sha1.hpp>

namespace xcal {

/**
 * @brief Helper class to generate sha1 checksum.
 *
 */
class Checksum {
public:
    /**
     * @brief Construct a new Checksum object
     *
     * @param s the initial string content to be hashed.
     */
    Checksum(const std::string &s = "");

    /**
     * @brief Process the content
     *
     * @param s the content.
     */
    void process(const std::string &s);

    /**
     * @brief Generate hex digest in string format.
     *
     * @return std::string the digest.
     */
    std::string output();

private:

    /// Internal data.
    boost::uuids::detail::sha1 h;
};

}
