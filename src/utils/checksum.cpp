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

#include <sstream>
#include <iomanip>

#include "checksum.hpp"

using namespace xcal;

Checksum::Checksum(const std::string &s) {
    if (s.empty()) {
        return;
    }

    this->process(s);
}

void Checksum::process(const std::string &s) {
    h.process_bytes(s.c_str(), s.size());
}

std::string Checksum::output() {
    unsigned int digest[5];
    h.get_digest(digest);

    std::ostringstream os;
    for (int i = 0; i < 5; ++i) {
        os << std::hex << std::setfill('0') << std::setw(8) << digest[i];
    }
    return os.str();
}