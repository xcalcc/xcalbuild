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
#include <mutex>
#include <unordered_set>
#include <set>
#include <fcntl.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include "taskflow/taskflow.hpp"

#include <nlohmann/json.hpp>

#include "build-processor.hpp"
#include "toolchain-profile.hpp"
#include "command-line.hpp"
#include "work-item-processor.hpp"
#include "prober.hpp"
#include "logger.hpp"
#include "profiler.hpp"
#include "error-code.hpp"
#include "checksum.hpp"
#include "archive.hpp"
#include "path.hpp"

namespace bp = boost::process;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
namespace io = boost::iostreams;
using json = nlohmann::json;

using namespace xcal;

BuildProcessor::BuildProcessor(
    ToolchainProfile &toolchain_profile,
    const CommandLine &command_line,
    int parallelism
):
    toolchain_profile(toolchain_profile),
    command_line(command_line),
    output_path(
        boost::filesystem::weakly_canonical(
            boost::filesystem::system_complete(
                command_line.get_output_dir()))),
    parallelism(parallelism) {
        //split keywork string with seperator to create vector
        if (command_line.get_fkey().size() > 1)
        {
            boost::split(filter_key, command_line.get_fkey(), boost::is_any_of(";"));
        }

        if (command_line.get_lkey().size() > 1)
        {
            boost::split(linker_key, command_line.get_lkey(), boost::is_any_of(";"));
        }
    }

boost::optional<ErrorCode> BuildProcessor::load_cdb(const fs::path &path) {
    // Load and parse the build spec generated.
    fs::ifstream ifs{ path };
    try {
        cdb = json::parse(ifs);
    } catch(...) {
        LOG_EC(critical, EC_ERROR_PARSING_CDB) << "Error parsing compile commands";
        return EC_ERROR_PARSING_CDB;
    }
    ifs.close();
    LOG(info) << "Number of compile commands:" << cdb.size();
    return boost::none;
}

const std::vector<std::string> &BuildProcessor::get_filter_key() const {
    return filter_key;
}

const std::vector<std::string> &BuildProcessor::get_linker_key() const {
    return linker_key;
}

const std::vector<std::string> &BuildProcessor::get_whitelist_files() const {
    return whitelist_files;
}

const std::vector<std::string> &BuildProcessor::get_blacklist_files() const {
    return blacklist_files;
}

std::string BuildProcessor::detect_toolchain_profile() {
    std::map<std::string, int> origin_counts;
    int count = 0;

    // Check profile origin of each work items.
    for(const json &work_item: cdb) {
        // Profile can be nullptr, but should be ok.
        auto profile = toolchain_profile.get_tool_profile(work_item);
        auto origins = toolchain_profile.get_profile_origins(profile);
        if(origins) {
            for(const auto &origin: *origins) {
                if(origin_counts.count(origin)) {
                    origin_counts[origin] = origin_counts[origin] + 1;
                } else {
                    origin_counts[origin] = 1;
                }
            }
        }
        ++count;
    }

    std::string res;
    int highest = 0;
    for(const auto &p: origin_counts) {
        if(p.second == count) {
            // Return if we have a full match.
            // I.E. all binaries traced are from a single profile.
            LOG(info) << "To use fully matched toolchain profile: " << p.first;
            return p.first;
        } else if(p.second > highest) {
            res = p.first;
            highest = p.second;
        }
    }

    LOG(info) << "To use the best partially matched (" << highest << "/" << count << ") toolchain profile: " << res;
    // Otherwise, return the one with most matches.
    return res;
}

boost::optional<ErrorCode> BuildProcessor::process() {
    // IO paths.
    auto compile_db_path = output_path / command_line.get_cdb_name();
    LOG(info) << "Compile Database Path: " << compile_db_path.string().c_str();

    // Whitelist command execution
    if (command_line.get_wlfcmd().size() > 1) {
        auto whitelist_file = (fs::system_complete(command_line.get_output_dir()) / fs::path("whitelist.txt")).string();
        LOG(info) << "whitelist file Path: " << whitelist_file.c_str();
        try {
            //bp::child b(command_line.get_wlfcmd(), ">", whitelist_file, bp::start_dir=command_line.get_build_dir());
            //b.wait();
            std::vector<std::string> args {"-c", command_line.get_wlfcmd() + " > " + whitelist_file};
            bp::system(bp::search_path("sh"), args, bp::start_dir=command_line.get_build_dir());
        } catch (const std::exception& err) {
        //} catch (boost::process::process_error &err) {
            LOG(warning) << "Failed to run whitelist cmd" << command_line.get_wlfcmd() << ", reason:" << err.what();
            return EC_ERROR_PARSING_CDB;
        }

        int fd = open(whitelist_file.c_str(), O_RDONLY);
        if (fd >= 0) {
            io::file_descriptor_source fdDevice(fd, io::file_descriptor_flags::close_handle);
            io::stream <io::file_descriptor_source> in(fdDevice);
            if (fdDevice.is_open()) {
                std::string line;
                while (std::getline(in, line)) {
                    whitelist_files.push_back(get_full_path_str(line, command_line.get_build_dir()));
                }
                fdDevice.close();
            }
        }
    } else {
        // Blacklist command execution
        if (command_line.get_blfcmd().size() > 1) {
            auto blacklist_file = (fs::system_complete(command_line.get_output_dir()) / fs::path("blacklist.txt")).string();
            try {
                std::vector<std::string> args {"-c", command_line.get_blfcmd() + " > " + blacklist_file};
                bp::system(bp::search_path("sh"), args, bp::start_dir=command_line.get_build_dir());
            } catch (const std::exception& err) {
                LOG(warning) << "Failed to run blacklist cmd" << command_line.get_blfcmd() << ", reason:" << err.what();
                return EC_ERROR_PARSING_CDB;
            }

            int fd = open(blacklist_file.c_str(), O_RDONLY);
            if (fd >= 0) {
                io::file_descriptor_source fdDevice(fd, io::file_descriptor_flags::close_handle);
                io::stream <io::file_descriptor_source> in(fdDevice);
                if (fdDevice.is_open()) {
                    std::string line;
                    while (std::getline(in, line)) {
                        blacklist_files.push_back(get_full_path_str(line, command_line.get_build_dir()));
                    }
                    fdDevice.close();
                }
            }
        }
    }

    Stopwatch watch;

    auto ec_opt = load_cdb(compile_db_path);
    if(ec_opt) {
        return ec_opt;
    }
    LOG(debug) << "CDB PARSING TIME: " << watch.get_duration();

    // Detect toolchain profile if needed.
    ToolchainProfile detected_toolchain_profile;
    auto detect_profile = command_line.is_auto_detect_toolchain_profile();
    if(detect_profile) {
        detected_toolchain_profile.load(command_line.get_tool_root() / "profiles" / detect_toolchain_profile());
        // Don't need to trim the CDB here since the build most likely
        // will only use one toolchain.
    }
    ToolchainProfile &actual_toolchain_profile = detect_profile ? detected_toolchain_profile : toolchain_profile;

    // Probe the toolchain, update profile.
    LOG(info) << "Probe the toolchain, update profile";
    Prober prober;
    prober.probe_toolchain(cdb, actual_toolchain_profile);

    // Load the actionable parts of the profile.
    LOG(info) << "Load the actionable parts of the profile";
    actual_toolchain_profile.load_actionable();

    LOG(debug) << "PROBE TIME: " << watch.get_duration();

    // Process the work items.
    LOG(info) << "Process work items";
    tf::Executor executor(parallelism);
    tf::Taskflow taskflow;
    taskflow.parallel_for(cdb.begin(), cdb.end(), [&](const auto &item) {
        WorkItemProcessor wip(actual_toolchain_profile, command_line, item, this);
        wip.process();
    });
    executor.run(taskflow).get();

    LOG(debug) << "PREPROCESSING TIME: " << watch.get_duration();

    ec_opt = generate_output();

    LOG(debug) << "OUTPUT TIME: " << watch.get_duration();

    return ec_opt;
}

struct LDInfo {
    fs::path pp_output_path;
    std::set<std::string> dependencies;
    std::vector<std::string> c_scan_options;
    bool c_scan_options_set = false;
    std::vector<std::string> cxx_scan_options;
    bool cxx_scan_options_set = false;
};

boost::optional<ErrorCode> BuildProcessor::generate_output() {
    // Clean and create top-level dir.
    auto pp_dir_name = command_line.get_preprocess_dir_name();
    auto pp_path = fs::path(pp_dir_name);
    auto pp_output_path = output_path / pp_path;
    auto property_file_path = fs::path("xcalibyte.properties");

    // The archive to output.
    Archive archive{pp_output_path};

    // Top-level xcalibyte.properties, relative path.
    archive.add_file(property_file_path, std::string());

    // Top-level checksum file to be written.
    std::ostringstream checksum_file;

    // Build a map from assembler target to source.
    // This is used to link ld source to cc target.
    std::map<std::string, std::string> as_target_to_source;
    for(const auto &as: as_results) {
        as_target_to_source[as.target] = as.source;
    }

    // Memory management for ld infos.
    std::vector<std::unique_ptr<LDInfo>> ld_infos;

    // Map from link source to targets,
    std::map<std::string, std::unordered_set<LDInfo *>> link_targets;

    // Maps used to deduplicate target dir names.
    // A map from seen target file names to duplication count.
    // e.g. with targets:
    //   /a/a.lib /b/a.lib /b/b.lib /c/a.lib /c/b.lib /c/c.lib
    // the final map would be:
    //   a.lib -> 3, b.lib -> 2, c.lib -> 1
    std::map<std::string, int> seen_target_filenames;

    // A map from target full path to the file name in the dependencies list.
    std::map<std::string, std::string> dependency_names;

    // Process link results.
    for(const auto &ld: ld_results) {
        // Make a new ld info for this target.
        ld_infos.emplace_back(new LDInfo());
        auto *ld_info = ld_infos.back().get();

        std::string filename = fs::path(ld.target).filename().string();
        auto it = seen_target_filenames.find(filename);
        if(it == seen_target_filenames.end()) {
            seen_target_filenames.insert(std::make_pair(filename, 1));
        } else {
            // Duplicated name.
            filename = filename + "." + std::to_string(it->second);
            // Increment the count;
            it->second += 1;
        }
	
	// Construct target absolute path
	auto target_path = fs::path(ld.target);
        for(const auto &source: ld.sources) {
	    auto orig_source_path = fs::path(source);
	    auto source_ext = orig_source_path.extension().string();
	    auto source_filename = orig_source_path.stem().string();
	    if(source_ext.compare(".o") == 0 && source_filename.compare(target_path.stem().string()) == 0) {
               target_path = orig_source_path.parent_path() / target_path.filename();
	       break;
	    }
	}

        // Update dep name cache.
        dependency_names.insert(std::make_pair(target_path.string(), filename));

        // Create output dirs, e.g. a.out.dir/preprocess
        ld_info->pp_output_path = fs::path(filename + ".dir") / pp_path;
        archive.add_dir(ld_info->pp_output_path);

        // Build the source-target relation.
        for(const auto &source: ld.sources) {
            // Check if the link source is a as target.
            // if so, substituite the source with as source to match cc target.
            std::string actual = source;
            if(as_target_to_source.count(source)) {
                actual = as_target_to_source[source];
            }

            // Put all source into dependencies, the actual sources will be removed later.
            ld_info->dependencies.insert(actual);

            if(link_targets.count(actual)) {
                link_targets[actual].insert(ld_info);
            } else {
                std::unordered_set<LDInfo *> s = { ld_info };
                // Move the string since no longer needed.
                link_targets.emplace(
                    std::make_pair(std::move(actual), std::move(s)));
            }
        }
    }

    // TODO: will remove hard code string later.
    if(command_line.need_process_link_using_compiler()) {
        ld_infos.emplace_back(new LDInfo());
        auto *ld_info = ld_infos.back().get();

        std::string filename = "test_ld"; //fs::path(ld.target).filename().string();
        auto it = seen_target_filenames.find(filename);
        if(it == seen_target_filenames.end()) {
            seen_target_filenames.insert(std::make_pair(filename, 1));
        } else {
            // Duplicated name.
            filename = filename + "." + std::to_string(it->second);
            // Increment the count;
            it->second += 1;
        }

        // Update dep name cache.
        //dependency_names.insert(std::make_pair(ld.target, filename));
        dependency_names.insert(std::make_pair("test_ld", filename));
        // Create output dirs, e.g. a.out.dir/preprocess
        ld_info->pp_output_path = fs::path(filename + ".dir") / pp_path;
        archive.add_dir(ld_info->pp_output_path);

        link_targets[filename].insert(ld_info);
    }

    // Source file could appear multiple times, deduplicate them.
    std::set<std::string> source_files;
    std::set<fs::path> pp_files;

    // Process cc results.
    for(const auto &cc: cc_results) {
        try {
            /*
            if(!link_targets.count(cc.target)) {
                LOG(warning) << "Compile target not linked: " << cc.target;
                fs::remove(cc.file.path);
                continue;
            }
            */

            // TODO: will remove hard code string later.
            std::string tempTargetName = cc.target;
            if(!link_targets.count(tempTargetName)) {
                if(command_line.need_process_link_using_compiler()) {
                    LOG(warning) << "Compile target linked through use of compiler command, use test_ld instead: " << tempTargetName;
                    tempTargetName = "test_ld"; 
		        } else {
                    LOG(warning) << "No need to process compile target linked through use of compiler command: " << cc.target;
                    fs::remove(cc.file.path);
                    continue;
		        }
            }

            source_files.insert(cc.source);
            source_files.insert(cc.deps.begin(), cc.deps.end());

            //auto &ld_infos = link_targets[cc.target];
            auto &ld_infos = link_targets[tempTargetName];

            for(LDInfo *ld_info: ld_infos) {
                auto path = ld_info->pp_output_path / fs::path(cc.pp_file_name);

                if(!pp_files.insert(path).second) {
                    // Append .x if same name file exists.
                    int idx = 1;
                    const auto orig_path = path;
                    auto ext = path.extension().string();
                    do {
                        path = fs::change_extension(orig_path, "."+std::to_string(idx)+ext);
                        ++idx;
                    } while (!pp_files.insert(path).second);
                }

                archive.add_file(path, cc.file.path);

                // Put it here to get the correct .i path.
                LOG(info) << "[SUCCESS]" << path.string() << " || " << cc.source;

                // Update checksum.
                checksum_file << cc.file.checksum << " " << (pp_output_path / path).string() << std::endl;

                // Remove the source as dependency.
                ld_info->dependencies.erase(cc.target);

                // Populate scan options.
                if(cc.format == FileFormat::FF_C_SOURCE && !ld_info->c_scan_options_set) {
                    ld_info->c_scan_options = std::move(cc.c_scan_options);
                    ld_info->c_scan_options_set = true;
                } else if(cc.format == FileFormat::FF_CXX_SOURCE && !ld_info->cxx_scan_options_set) {
                    ld_info->cxx_scan_options = std::move(cc.cxx_scan_options);
                    ld_info->cxx_scan_options_set = true;
                }
            }
            // Remove original copy of pp output file.
            fs::remove(cc.file.path);
        // GCOVR_EXCL_START
        } catch(std::exception &e) {
            LOG(warning) << "Failed to process: " << cc.file.path << ", target: " << cc.target << "\n" << e.what();
        }
        // GCOVR_EXCL_STOP
    }

    // Now create the xcalibyte.properties files.
    for(auto &ld_info: ld_infos) {
        auto properties = command_line.get_properties_template();

        // Replace dependencies with deduplicated file name.
        std::vector<std::string> dependencies;
        for(auto &dep: ld_info->dependencies) {
            if(dependency_names.count(dep)) {
                dependencies.push_back(std::move(dependency_names[dep]));
            } else {
                dependencies.push_back(std::move(dep));
            }
        }
        properties.put("dependencies", ba::join(dependencies, " "));
        properties.put("c_scan_options", ba::join(ld_info->c_scan_options, " "));
        properties.put("cxx_scan_options", ba::join(ld_info->cxx_scan_options, " "));

        // Add property file to archive.
        std::ostringstream oss;
        boost::property_tree::write_ini(oss, properties);
        auto path = ld_info->pp_output_path.parent_path() / property_file_path;
        archive.add_file(path, oss.str());

        // E.g.:
        // 8abda020db9cdbce05e2afb3a702b88cd91af4f6  /xcal/preprocess/test_null.dir/xcalibyte.properties
        Checksum checksum(oss.str());
        checksum_file << checksum.output() << " " << (pp_output_path / path).string() << std::endl;
    }

    archive.add_file("checksum.sha1", checksum_file.str());

    json srcs = json::array();
    for(const auto &src: source_files) {
        srcs.push_back(std::move(src));
    }
    fs::ofstream src_file_json{ output_path / command_line.get_source_list_file_name() };
    src_file_json << srcs.dump() << std::endl;
    src_file_json.close();

    return boost::none;
}

void BuildProcessor::append_cc_result(CCResult &&result) {
    const std::lock_guard<std::mutex> lock(cc_results_mutex);
    cc_results.emplace_back(result);
}

void BuildProcessor::append_as_result(ASResult &&result) {
    const std::lock_guard<std::mutex> lock(as_results_mutex);
    as_results.emplace_back(result);
}

void BuildProcessor::append_ld_result(LDResult &&result) {
    const std::lock_guard<std::mutex> lock(ld_results_mutex);
    ld_results.emplace_back(result);
}
