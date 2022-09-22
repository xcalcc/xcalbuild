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

#include <boost/log/trivial.hpp>
#include <boost/log/common.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace xcal {

enum LogSeverityLevel {
    info = 0,
    debug = 1,
    warning = 2,
    error = 3,
    critical = 4
};

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(slg, boost::log::sources::severity_logger<LogSeverityLevel>)

// file name without absolute path
#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG(level) BOOST_LOG_SEV(slg::get(), level) \
	<< boost::log::add_value("File",__FILENAME__) \
	<< boost::log::add_value("Function",__FUNCTION__)

#define LOG_EC(level, ec) BOOST_LOG_SEV(slg::get(), level) \
	<< boost::log::add_value("ErrorCode", ec) \
	<< boost::log::add_value("File",__FILENAME__) \
	<< boost::log::add_value("Function",__FUNCTION__)

/**
 * @brief Global initialisation.
 *
 * @param file_name the log file name
 * @param trace_id trace id passed over from the calling entity
 * @param span_id span id passed over from the calling entity
 * @param debug whether to output debug log
 * @param logging_file whether to output log to standard output (to be consumed by the calling entity) or file
 */
void init_logger(const std::string &file_name, const std::string &trace_id, const std::string &span_id, bool debug = false, bool logging_file = false);

}