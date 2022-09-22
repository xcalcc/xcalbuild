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

/*
    Need to follow the ec standard of Xcal:

    For error coding, we need the following information in an excel table

    * Who could resolve the issue
        General, internal, external, user (0, 1, 2, 3) (external is usually network, external web access etc)
    * Where during the scan process the problem arises
        For Excalbuild, it will be 3
        For set up (which target compiler etc when user need to provide the info it will be 2)
    * Which party is the cause of the issue
        User, external, not used, xcal-internal (0, 1, 2, 3) (external is usually things like network problem, disk space, …)
    * What is the problem
        Internally assigned number (you folks can use your own, we will do the merging). Please provide a brief description of the problem.
    * Is it visible to customer
        yes, no (1, 0)
*/

namespace xcal {

// Who could resolve the issue
#define EC_ASSIGNEE_GENERAL 0x0
#define EC_ASSIGNEE_INTERNAL 0x1
#define EC_ASSIGNEE_EXTERNAL 0x2
#define EC_ASSIGNEE_USER 0x3

// Where during the scan process the problem arises
#define EC_PROCESS_SETUP 0x2
#define EC_PROCESS_XCALBUILD 0x3

// Which party is the cause of the issue
#define EC_CAUSE_USER 0x0
#define EC_CAUSE_EXTERNAL 0x1
#define EC_CAUSE_NOT_USED 0x2
#define EC_CAUSE_INTERNAL 0x3

// Is it visible to customer
#define EC_VISIBLE 0x0
#define EC_NOT_VISIBLE 0x1

// Generate the exit code.
#define GEN_EC(assignee, process, cause, visibility, desc) \
    ((assignee << 20) + ((process) << 16) + ((cause) << 12) + ((visibility) << 8) + (desc))

// What is the problem, also the actual exit code.
enum ErrorDescription {
    EC_DESC_UNKOWN = 0x1,
    EC_DESC_INCORRECT_COMMAND_LINE,
    EC_DESC_ERROR_PARSING_CDB,
    EC_DESC_INCORRECT_CONFIG_FILE,
    EC_DESC_INCORRECT_TOOLCHAIN_PROFILE,
    EC_DESC_ARCHIVE_ERROR,
    EC_DESC_STRACE_NOT_FOUND,
    EC_DESC_COMPILATION_FAILURE,
    EC_DESC_BEAR_NOT_FOUND,
};

// Might also define this to EXIT_FAILURE
#define EXIT_CODE(ec) (ec & 0xFF)

// The actual codes
enum ErrorCode {
    EC_NONE = 0x0,
    EC_UNKOWN = GEN_EC(EC_ASSIGNEE_INTERNAL, EC_PROCESS_XCALBUILD, EC_CAUSE_INTERNAL, EC_VISIBLE, EC_DESC_UNKOWN),
    EC_INCORRECT_COMMAND_LINE = GEN_EC(EC_ASSIGNEE_INTERNAL, EC_PROCESS_XCALBUILD, EC_CAUSE_INTERNAL, EC_VISIBLE, EC_DESC_INCORRECT_COMMAND_LINE),
    EC_ERROR_PARSING_CDB = GEN_EC(EC_ASSIGNEE_INTERNAL, EC_PROCESS_XCALBUILD, EC_CAUSE_INTERNAL, EC_VISIBLE, EC_DESC_ERROR_PARSING_CDB),
    EC_INCORRECT_CONFIG_FILE = GEN_EC(EC_ASSIGNEE_INTERNAL, EC_PROCESS_XCALBUILD, EC_CAUSE_INTERNAL, EC_VISIBLE, EC_DESC_INCORRECT_CONFIG_FILE),
    EC_INCORRECT_TOOLCHAIN_PROFILE = GEN_EC(EC_ASSIGNEE_USER, EC_PROCESS_XCALBUILD, EC_CAUSE_USER, EC_VISIBLE, EC_DESC_INCORRECT_TOOLCHAIN_PROFILE),
    EC_ARCHIVE_ERROR = GEN_EC(EC_ASSIGNEE_INTERNAL, EC_PROCESS_XCALBUILD, EC_CAUSE_INTERNAL, EC_VISIBLE, EC_DESC_ARCHIVE_ERROR),
    EC_STRACE_NOT_FOUND = GEN_EC(EC_ASSIGNEE_USER, EC_PROCESS_XCALBUILD, EC_CAUSE_USER, EC_VISIBLE, EC_DESC_STRACE_NOT_FOUND),
    EC_COMPILATION_FAILURE = GEN_EC(EC_ASSIGNEE_USER, EC_PROCESS_XCALBUILD, EC_CAUSE_USER, EC_VISIBLE, EC_DESC_COMPILATION_FAILURE),
    EC_BEAR_NOT_FOUND = GEN_EC(EC_ASSIGNEE_USER, EC_PROCESS_XCALBUILD, EC_CAUSE_USER, EC_VISIBLE, EC_DESC_BEAR_NOT_FOUND),
};

}
