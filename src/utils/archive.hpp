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

#include <boost/filesystem.hpp>

struct archive;

namespace xcal {

/**
 * @brief An output archive
 *
 */
class Archive {
public:
    /**
     * @brief Construct a new Archive object
     *
     * @param dir the dir name of the tar ball
     */
    Archive(const boost::filesystem::path &dir);
    ~Archive();

    /**
     * @brief Add a directory to the tar ball
     *
     * @param dir the directory
     */
    void add_dir(const boost::filesystem::path &dir);

    /**
     * @brief Add a file to the tar ball
     *
     * @param file the file path
     * @param content the file content in string
     */
    void add_file(const boost::filesystem::path &file, const std::string &content);

    /**
     * @brief Add a file to the tar ball
     *
     * @param file the file path
     * @param content_file the file whose content is to be added
     */
    void add_file(const boost::filesystem::path &file, const boost::filesystem::path &content_file);

private:
    /**
     * @brief Add an entry header to archive
     *
     * @param path the path
     * @param is_file whether a file or a dir
     * @param size the size of the entry
     */
    void add_header(const char *path, bool is_file, size_t size);

    /// The archive object.
    struct archive *a;
};

}
