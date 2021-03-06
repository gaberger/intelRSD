/*!
 * @section LICENSE
 *
 * @copyright
 * Copyright (c) 2015-2017 Intel Corporation
 *
 * @copyright
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * @copyright
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * @copyright
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @section DESCRIPTION
 * */

#include "agent-framework/module/compute_components.hpp"
#include "agent-framework/module/common_components.hpp"
#include "agent-framework/command-ref/registry.hpp"
#include "agent-framework/command-ref/compute_commands.hpp"

using namespace agent_framework::command_ref;
using namespace agent_framework::module;
using namespace agent_framework::model;

namespace {
    void process_collections(StorageController& storage_controller) {
        if (CommonComponents::get_instance()->
                get_drive_manager().get_entry_count(storage_controller.get_uuid()) > 0) {
            storage_controller.add_collection(attribute::Collection(enums::CollectionName::Drives,
                    enums::CollectionType::Drives,""));
        }
    }
}

REGISTER_COMMAND(GetStorageControllerInfo,
    [] (const GetStorageControllerInfo::Request& req, GetStorageControllerInfo::Response& rsp) {
        log_debug(GET_LOGGER("agent"), "Getting storage controller info.");
        rsp = ComputeComponents::get_instance()->
            get_storage_controller_manager().get_entry(req.get_uuid());
        ::process_collections(rsp);
    }
);
