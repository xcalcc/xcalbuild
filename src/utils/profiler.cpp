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

#include "profiler.hpp"

namespace bpt = boost::posix_time;
using namespace xcal;

xcal::Stopwatch::Stopwatch() {
    tick = bpt::microsec_clock::local_time();
}

float xcal::Stopwatch::get_duration(bool reset) {
    auto now = bpt::microsec_clock::local_time();
    auto res = ((now - tick).total_milliseconds() / 1000.0);
    if(reset) {
        tick = now;
    }
    return res;
}
