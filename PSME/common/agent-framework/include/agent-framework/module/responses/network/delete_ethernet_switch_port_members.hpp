/*!
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
 *
 * @file responses/network/delete_ethernet_switch_port_members.hpp
 * @brief Definition of network DeleteEthernetSwitchPortMembers response
 * */

#pragma once
#include "agent-framework/module/model/attributes/oem.hpp"
#include <string>

namespace Json {
    class Value;
}

namespace agent_framework {
namespace model {
namespace responses {

/*! DeleteEthernetSwitchPortMembers request */
class DeleteEthernetSwitchPortMembers  {
public:
    using Oem = agent_framework::model::attribute::Oem;

    /*!
     * @brief explicit DeleteEthernetSwitchPortMembers response constructor
     */
    explicit DeleteEthernetSwitchPortMembers(Oem oem = Oem{});

    /*!
     * @brief Transform request to Json
     *
     * @return created Json value
     */
    Json::Value to_json() const;

    /*!
     * @brief create DeleteEthernetSwitchPortMembers from Json
     *
     * @param[in] json the input argument
     *
     * @return new DeleteEthernetSwitchPortMembers
     */
    static DeleteEthernetSwitchPortMembers from_json(const Json::Value& json);

    /*!
     * @brief Set oem object
     *
     * @param[in] oem Oem object to set
     *
     */
    void set_oem(const Oem& oem) { m_oem = oem; }

private:
    Oem m_oem{};
};

}
}
}
