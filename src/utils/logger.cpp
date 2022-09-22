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

#include <iomanip>
#include <iostream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>

#include "logger.hpp"
#include "fwd.hpp"
#include "error-code.hpp"
#include "command-line.hpp"

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace pt = boost::posix_time;

using namespace xcal;

struct severity_tag;

constexpr const char* log_level_names[] = {
    "INFO",
    "DEBUG",
    "WARNING",
    "ERROR",
    "CRITICAL"
};
CHECK_NAMES_SIZE(log_level_names, LogSeverityLevel::critical);

logging::formatting_ostream &operator<< (
    logging::formatting_ostream &os,
    logging::to_log_manip<LogSeverityLevel, severity_tag> const &manip)
{
    LogSeverityLevel level = manip.get();
    os << log_level_names[static_cast<std::size_t>(level)];
    return os;
}

// Print local time, instead of using TimeStamp
class local_time_impl :
    public logging::attribute::impl
{
public:
    // The method generates a new attribute value
    logging::attribute_value get_value()
    {
        //boost::local_time::local_date_time dt{pt::second_clock::universal_time(), tz};
        boost::local_time::local_date_time dt{pt::microsec_clock::universal_time(), tz};
        //boost::local_time::local_date_time dt{pt::microsec_clock::local_time()};
        return attrs::make_attribute_value(dt.local_time());
    }

    boost::local_time::time_zone_ptr tz{new boost::local_time::posix_time_zone{"UTC+8"}};
};

class local_time :
    public logging::attribute
{
public:
    local_time() : logging::attribute(new local_time_impl())
    {
    }
    // Attribute casting support
    explicit local_time(attrs::cast_source const& source) : logging::attribute(source.as< local_time_impl >())
    {
    }
};

void xcal::init_logger(const std::string &file_name, const std::string &trace_id, const std::string &span_id, bool debug, bool logging_file)
{
    auto format = (
        // time(date,time-hhmmss.ms), level, service, trace id, span id, span, process id, thread name, file name, severity, message (, error_code: 0x0000xxxx)
        expr::stream
        << expr::format_date_time<boost::posix_time::ptime>("LocalTime", "%Y-%m-%d %H:%M:%S.%f") << ", " // logging time including milliseconds
        << std::setw(8) << std::right << std::uppercase << expr::attr<LogSeverityLevel, severity_tag>("Severity") << ", " // logging level in capital, see log_level_names [info, debug, error...]
        << "xcalbuild, " // logging service
        //<< std::setfill('0') << std::setw(16) << std::right << std::hex << expr::attr<std::string>("Trace ID") << ", " //trace id
        << std::setfill('0') << std::setw(16) << std::right << std::hex << expr::attr<std::string>("Span ID") << ", " //span id
        << std::setfill(' ') << std::setw(25) << std::right << expr::attr<std::string>("File") << ", " // file name (thread name)
        << std::setfill(' ') << std::setw(30) << std::right << expr::attr<std::string>("Function") << ", " // function (file name, class name with 40 characters)
        << expr::smessage // message
        << expr::if_(expr::has_attr<ErrorCode>("ErrorCode")) // error if there is any
           [
               // if "ErrorCode" is present then put it to the record
               expr::stream << ", error_code: " <<  "0x" << std::setfill('0') << std::setw(8) << std::right << std::hex << expr::attr<ErrorCode>("ErrorCode")
           ]
    );

    // when logging_file mode is false, logging is directed to standard output, which could also be consumed by the calling entity
    // when logging file mode is true, logging is directed to log file
    if(logging_file == false) {
        logging::add_console_log(
            std::cout,
            keywords::auto_flush = true,
            keywords::format = format);
    } else {
        logging::add_file_log(
            keywords::file_name = file_name,
            keywords::auto_flush = true,
            keywords::format = format);
    }

    logging::core::get()->add_global_attribute("LocalTime", local_time());
    //logging::core::get()->add_global_attribute("Trace ID", attrs::constant<std::string>(trace_id));
    logging::core::get()->add_global_attribute("Span ID", attrs::constant<std::string>(span_id));

    // if(!debug) {
    //     logging::core::get()->set_filter(
    //         boost::log::expressions::attr<LogSeverityLevel>("Severity") > LogSeverityLevel::debug);
    // }

    logging::add_common_attributes();
}
