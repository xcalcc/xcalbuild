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

#include <string>

#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

#include "fwd.hpp"

namespace xcal {

/**
 * @brief Process a single work item
 * Write the result back to the queues in build processor.
 */
class WorkItemProcessor {
public:
    /**
     * @brief Construct a new Work Item Processor object
     *
     * @param toolchain_profile The toolchain profile to be used.
     * @param work_item The work item to be processed.
     * @param build_processor The parent build processor.
     */
    WorkItemProcessor(
        const ToolchainProfile &toolchain_profile,
        const CommandLine &command_line,
        const nlohmann::json &work_item,
        BuildProcessor *build_processor
    );

    /**
     * @brief Process the work item.
     *
     */
    void process();

    /**
     * @brief Handle a link work item.
     *
     * @param parsed the work item
     */
    void handle_link(ParsedWorkItem &parsed);

    /**
     * @brief Handle a compile work item.
     *
     * @param profile the tool profile to use
     * @param parsed the work item
     */
    void handle_compile(const ToolProfile *profile, ParsedWorkItem &parsed);

private:
    /// The toolchain profile to be used.
    const ToolchainProfile &toolchain_profile;
    /// The work item to be processed.
    const nlohmann::json &work_item;
    /// The parent build processor.
    BuildProcessor *build_processor;
    /// The output path.
    boost::filesystem::path output_dir;
};

}
