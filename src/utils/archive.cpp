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

#include <fcntl.h>

#include <sstream>

#include <archive.h>
#include <archive_entry.h>

#include "archive.hpp"

namespace fs = boost::filesystem;

using namespace xcal;

Archive::Archive(const boost::filesystem::path &dir) {
    auto output = fs::change_extension(dir, ".tar.gz");
    a = archive_write_new();
    archive_write_add_filter_gzip(a);
    // archive_write_set_options(a, "compression=9");
    archive_write_set_format_pax_restricted(a);
#if _WIN32
    archive_write_open_filename_w(a, output.c_str());
#else
    archive_write_open_filename(a, output.c_str());
#endif
}

Archive::~Archive() {
    archive_write_close(a);
    archive_write_free(a);
}

void Archive::add_header(const char *path, bool is_file, size_t size) {
    struct archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, path);
    archive_entry_set_filetype(entry, is_file ? AE_IFREG : AE_IFDIR);
    archive_entry_set_perm(entry, is_file ? 0644 : 0777);
    archive_entry_set_size(entry, size);
    archive_write_header(a, entry);
    archive_entry_free(entry);
}

void Archive::add_dir(const boost::filesystem::path &dir) {
    add_header(dir.string().c_str(), /*is_file*/false, /*size*/0);
}

void Archive::add_file(const boost::filesystem::path &file, const std::string &content) {
    auto size = content.length();
    add_header(file.string().c_str(), /*is_file*/true, /*size*/size);
    archive_write_data(a, content.c_str(), size);
}

void Archive::add_file(const boost::filesystem::path &file, const boost::filesystem::path &content_file) {
    fs::ifstream ifs{content_file};
    std::ostringstream oss;
    oss << ifs.rdbuf();
    add_file(file, oss.str());
}
