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

namespace xcal {

/**
 * @brief Tracing method.
 *
 */
enum TracingMethod {
    BEAR,
    STRACE,
    WINDBG
};

/**
 * @brief Detected source language.
 *
 */
enum FileFormat {
    FF_C_SOURCE = 0,
    FF_CXX_SOURCE = 1,
    FF_PREPROCESSED = 2,
    FF_ASSEMBLY = 3,
    FF_OBJECT = 4,
    FF_LIBRARY = 5,
    FF_EXECUTIVE = 6,
    /// By the file extension. Last.
    FF_EXT = 7
};

class BuildProcessor;
class ToolProfile;
class ToolchainProfile;
class CommandLine;
class Prober;
struct ParsedWorkItem;

#define CHECK_NAMES_SIZE(arr, last) \
    static_assert((sizeof(arr)/sizeof(const char *)) == (last) + 1, "Name array size mismatch")

}