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

#include <boost/process.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <nlohmann/json.hpp>

#include "work-item-processor.hpp"
#include "toolchain-profile.hpp"
#include "tool-profile.hpp"
#include "command-line.hpp"
#include "build-processor.hpp"
#include "checksum.hpp"
#include "logger.hpp"
#include "path.hpp"

namespace bp = boost::process;
namespace ba = boost::algorithm;
namespace fs = boost::filesystem;
using json = nlohmann::json;

using namespace xcal;

WorkItemProcessor::WorkItemProcessor(
    const ToolchainProfile &toolchain_profile,
    const CommandLine &command_line,
    const nlohmann::json &work_item,
    BuildProcessor *build_processor
):
    toolchain_profile(toolchain_profile),
    work_item(work_item),
    build_processor(build_processor),
    output_dir(boost::filesystem::system_complete(command_line.get_output_dir()))
{}

void WorkItemProcessor::handle_link(ParsedWorkItem &parsed) {
    if(!parsed.target.length()) {
        if(parsed.kind == CommandKind::CK_LINK) {
            parsed.target = "DEFAULT_OUTPUT";
        } else {
            LOG(warning) << "Archive target is empty\n" << parsed.to_json().dump(2); //GCOVR_EXCL_LINE
        }
    }

    std::string target_name;
    int pos = 0;
    #if _WIN32
        pos = parsed.target.find_last_of('\\');
    #else
        pos = parsed.target.find_last_of('/');
    #endif
    target_name = parsed.target.erase(0, pos + 1);

    if(!build_processor->get_linker_key().empty()) {
        bool bFound = (std::find(build_processor->get_linker_key().begin(), build_processor->get_linker_key().end(), target_name) != build_processor->get_linker_key().end());
        // the first given position is the string black, which is the blacklist. In other cases, it is the whitelist.
        bool bBlackList = (std::find(build_processor->get_linker_key().begin(), build_processor->get_linker_key().end(), "black") == build_processor->get_linker_key().begin());
        // black list || white list
        if ((bBlackList && bFound) || (!bBlackList && !bFound))
        {
            LOG(info) << "Link target " << target_name << " filter out";
            return;
        }
    }

    std::vector<std::string> sources;
    for(const auto &source: parsed.sources) {
        sources.push_back(std::move(source.first));
    }

    LDResult res(std::move(parsed.target), std::move(sources));
    build_processor->append_ld_result(std::move(res));
}

void WorkItemProcessor::handle_compile(const ToolProfile *profile, ParsedWorkItem &parsed) {
    bool multi_sources = parsed.sources.size() > 1;
    std::vector<std::pair<std::string, FileFormat>> pseudo_link_sources;

    // If no target specified, create a pesudo one.
    if(parsed.target == "") {
        parsed.target = get_temp_path(&output_dir).string();
    }

    // Split into per TU command lines.
    for(const auto &source: parsed.sources) {
        // TODO: there could be duplicated pp command lines, might want to deduplicate
        // before firing the child process.
        auto file = source.first;
        auto format = source.second;
        auto file_path = fs::path(file);

        if(!fs::exists(file_path)) {
            LOG(warning) << "Source file " << file_path << " no longer exists, ignored.";
            continue;
        }

        auto dir_path = fs::path(parsed.dir);
        auto filename = file_path.filename().string();

        // Put the temp in output dir to avoid fs link issues, etc.
        auto temp_path = get_temp_path(&output_dir);

        if(format == FileFormat::FF_C_SOURCE) {
            filename += ".i";
            temp_path = temp_path.replace_extension(".i");
            parsed.cxx_scan_options.clear();
        } else if(format == FileFormat::FF_CXX_SOURCE) {
            filename += ".ii";
            temp_path = temp_path.replace_extension(".ii");
            parsed.c_scan_options.clear();
        }

        auto temp = temp_path.string();

        auto opts = profile->get_preprocessing_options(temp, format, parsed);

#if _WIN32
        // Some older build tools don't give us the .exe file extension we need
        // to call them again. This makes sure all executables end in .exe
        parsed.binary = fs::path(parsed.binary).replace_extension(".exe").string();
#endif
        LOG(info) << "Running: " <<
            get_full_path_str(parsed.binary, parsed.dir, /*search*/true) << " " <<
            ba::join(opts, " ") << " " <<
            file << std::endl <<
            "dir: " << parsed.dir << ", target : " << parsed.target << std::endl;

        int ec = 0;
        try {
            bp::child pp(
                get_full_path_str(parsed.binary, parsed.dir, /*search*/true),
                bp::args(opts),
                file,
                bp::start_dir=dir_path);

            pp.wait(); //wait for the process to exit
            ec = pp.exit_code();
        //GCOVR_EXCL_START
        } catch (boost::process::process_error &err) {
            LOG(warning) << "Failed to run " << parsed.binary << ", reason:" << err.what();
            ec = 1;
        }
        //GCOVR_EXCL_STOP

        //GCOVR_EXCL_START
        if(ec != 0) {
            auto first_file = parsed.sources[0].first;
            auto first_name =
                parsed.sources[0].second == FileFormat::FF_C_SOURCE
                ? first_file + ".i" : first_file + ".ii";
            LOG(info) << "[FAIL]" << first_name << " || " << first_file;
            LOG(info) << parsed.to_json().dump(2);
            fs::remove_all(temp);
            return;
        }
        //GCOVR_EXCL_STOP

        // Read in pp output.
        fs::ifstream pp_in{ temp_path };
        std::stringstream pp_orig;
        pp_orig << pp_in.rdbuf();
        pp_in.close();

        // Get files for source_files.json.
        std::set<std::string> deps;
        for (std::string line; std::getline(pp_orig, line); ) {
            // Source file name and line number information is conveyed by lines of the form
            //    # linenum filename flags
            // After the file name comes zero or more flags, which are 1, 2, 3, or 4.
            // If there are multiple flags, spaces separate them.
            //    1: This indicates the start of a new file.
            // e.g.:
            //    # 1 "./config.h" 1
            // We only want directives with flag 1.
            if(line[0] == '#' && line[line.length()-1] == '1') {
                std::vector<std::string> v;
                ba::split(v, line, boost::is_any_of("\""));
                if(v.size() == 3 && !ba::starts_with(v[1], "<")) {
                    deps.insert(get_full_path_str(v[1], parsed.dir));
                }
            }
        }

        // Text substitution
        auto replaced = profile->process_source_code(pp_orig.str());

        // Geneate checksum.
        Checksum checksum(replaced);

        // Write back.
        fs::ofstream pp_out{ temp_path };
        pp_out << replaced;
        pp_out.close();

        ResultFile res_file(std::move(temp_path), checksum.output());

        auto target = parsed.target;
        bool target_is_directory = fs::is_directory(target);

        //GCOVR_EXCL_START for now this is IAR only on windows.
        if (target_is_directory) {
            fs::path p(file);
            fs::path objPath = target / p.filename().replace_extension(".o");
            target = objPath.string();
        }
        //GCOVR_EXCL_STOP

        if(multi_sources && !target_is_directory) {
            target = fs::system_complete(fs::unique_path()).string();
            pseudo_link_sources.emplace_back(target, FileFormat::FF_OBJECT);
        }

        CCResult res(
            std::move(res_file),
            std::move(target),
            std::move(file),
            format,
            std::move(filename),
            std::move(parsed.c_scan_options),
            std::move(parsed.cxx_scan_options),
            std::move(deps));
        build_processor->append_cc_result(std::move(res));
    }

    // In case of multiple sources, create a pseudo link result to bridge
    // parsed.target with individual sources.
    if(multi_sources) {
        LOG(info) << "Multiple source files on the compile command line for '"
                  << parsed.target << "', creating pesudo link result";

        ParsedWorkItem l;
        l.kind = CommandKind::CK_LINK;
        l.target = parsed.target;
        l.sources = std::move(pseudo_link_sources);
        handle_link(l);
    }
}

void WorkItemProcessor::process() {
    auto profile = toolchain_profile.get_tool_profile(work_item);
    if(!profile) {
        LOG(warning) << "Binary not recognized.\n"  << work_item.dump(2);
        return;
    }

    ParsedWorkItem parsed;

    //Filter out unnecssary dir
    std::string lastdir_name;
    std::string work_dir = work_item["directory"].get<std::string>();

    int pos = 0;
    #if _WIN32
        pos = work_dir.find_last_of('\\');
    #else
        pos = work_dir.find_last_of('/');
    #endif
        lastdir_name = work_dir.erase(0, pos + 1);
    
    if(!build_processor->get_filter_key().empty()) {
        bool bFound = (std::find((build_processor->get_filter_key()).begin(), build_processor->get_filter_key().end(), lastdir_name) != build_processor->get_filter_key().end());
        // the first given position is the string black, which is the blacklist. In other cases, it is the whitelist.
        bool bBlackList = (std::find((build_processor->get_filter_key()).begin(), build_processor->get_filter_key().end(), "black") == build_processor->get_filter_key().begin());
        // black list || white list
        if ((bBlackList && bFound) || (!bBlackList && !bFound))
        {
            LOG(info) << "Directory " << work_dir << " filtered.";
            return;
        }
    }

    auto ec_opt = profile->parse_work_item(work_item, parsed);

    if(ec_opt || parsed.kind == CommandKind::CK_IGNORE) {
        LOG(info) << "IGNORED: Command " << work_item["arguments"].dump();
        return;
    }

    // All commands need source files.
    //GCOVR_EXCL_START this actually happens in eval.
    if(!parsed.sources.size()) {
        LOG(info) << "Command has no source" << std::endl
                  << work_item.dump(2) << std::endl
                  << parsed.to_json().dump(2);
        return;
    }
    //GCOVR_EXCL_STOP

    switch(parsed.kind) {
    case CommandKind::CK_COMPILE: {
        if(!build_processor->get_whitelist_files().empty()) {
            bool bFound = (std::find(build_processor->get_whitelist_files().begin(), build_processor->get_whitelist_files().end(), (parsed.sources.begin())->first) != build_processor->get_whitelist_files().end());
            if (!bFound) {
                break;
            }
        }

        if(!build_processor->get_blacklist_files().empty()) {
            bool bFound = (std::find(build_processor->get_blacklist_files().begin(), build_processor->get_blacklist_files().end(), (parsed.sources.begin())->first) != build_processor->get_blacklist_files().end());
            if (bFound) {
                break;
            }
        }
        handle_compile(profile, parsed);
        break;
    }
    case CommandKind::CK_ASSEMBLE: {
        ASResult res(
            std::move(parsed.target),
            std::move(parsed.sources[0]).first);
        build_processor->append_as_result(std::move(res));
        break;
    }
    case CommandKind::CK_ARCHIVE: // Link
    case CommandKind::CK_LINK:
        handle_link(parsed);
        break;
    //GCOVR_EXCL_START just to keep switch complete, the case is handled earlier.
    case CommandKind::CK_IGNORE:
        // Do nothing.
        break;
    }
    //GCOVR_EXCL_STOP
}
