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

#include <boost/process.hpp>

#include "path.hpp"

namespace fs = boost::filesystem;
namespace bp = boost::process;

std::string xcal::get_full_path_str(const std::string &path_str, const std::string &dir_str, bool search) {
    if(search && fs::path(path_str).filename() == fs::path(path_str)) {
        return bp::search_path(path_str).string();
    } else {
        // Attention: on macOS, fs::weakly_canonical(fs::complete(/xx, /yy)) method has different behavior with linux when path_str starts with /xx while dir_str doesn't start with /xx.
        // For example:
        // On macOS:
        // 1. path_str: /work/arraylist.c, dir_str: /work, fs::weakly_canonical(fs::complete(path_str, dir_str)).string(): /work/arraylist.c
        // 2. path_str: test1.c, dir_str: /work, fs::weakly_canonical(fs::complete(path_str, dir_str)).string(): /work/test1.c
        // 3. path_str: /tmp/123abc.s, dir_str: /work, fs::weakly_canonical(fs::complete(path_str, dir_str)).string(): /private/tmp/123abc.s
        
        // On Linux:
        // 1. path_str: /work/arraylist.c, dir_str: /work, fs::weakly_canonical(fs::complete(path_str, dir_str)).string(): /work/arraylist.c
        // 2. path_str: test1.c, dir_str: /work, fs::weakly_canonical(fs::complete(path_str, dir_str)).string(): /work/test1.c
        // 3. path_str: /tmp/123abc.s, dir_str: /work, fs::weakly_canonical(fs::complete(path_str, dir_str)).string(): /tmp/123abc.s
        
        // The number 3 is different in the above example. This make test case test_as_profile fail on macOS.
        // Add special process to make the result same with linux.
        #ifdef __APPLE__
            if((path_str.find("/") == 0) && path_str.find(dir_str) != 0) {
                return path_str;
            }
        #endif
        return fs::weakly_canonical(fs::complete(path_str, dir_str)).string();
    }
}

fs::path xcal::get_temp_path(const fs::path *base) {
    return (base ? *base : fs::temp_directory_path()) / fs::unique_path();
}
