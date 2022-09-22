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
#include <boost/date_time/posix_time/posix_time.hpp>

namespace xcal {

/**
 * @brief Helper function to print timing info.
 *
 */
class Stopwatch {
public:
    /**
     * @brief Construct a new Stopwatch object
     *
     */
    Stopwatch();

    /**
     * @brief Get the duration from beginning or the last reset
     *
     * @param reset if true, reset the watch
     * @return float the time duration in seconds.
     */
    float get_duration(bool reset = true);

private:
    /// Internal data.
    boost::posix_time::ptime tick;
};

}