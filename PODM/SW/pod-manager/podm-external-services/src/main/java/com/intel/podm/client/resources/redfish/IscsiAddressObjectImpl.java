/*
 * Copyright (c) 2016-2017 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.podm.client.resources.redfish;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.intel.podm.client.api.resources.redfish.IscsiAddressObject;
import com.intel.podm.client.api.resources.redfish.ChapObject;
import com.intel.podm.client.api.resources.redfish.TargetLunObject;

import java.util.ArrayList;
import java.util.List;

public class IscsiAddressObjectImpl implements IscsiAddressObject {
    @JsonProperty("TargetLUN")
    List<TargetLunObjectImpl> targetLuns = new ArrayList<>();

    @JsonProperty("TargetIQN")
    private String targetIqn;

    @JsonProperty("TargetPortalIP")
    private String targetPortalIp;

    @JsonProperty("TargetPortalPort")
    private Integer targetPortalPort;

    @JsonProperty("CHAP")
    private ChapObjectImpl chap;

    @Override
    public List<TargetLunObject> getTargetLuns() {
        return (List) targetLuns;
    }

    @Override
    public String getTargetIqn() {
        return targetIqn;
    }

    @Override
    public String getTargetPortalIp() {
        return targetPortalIp;
    }

    @Override
    public Integer getTargetPortalPort() {
        return targetPortalPort;
    }

    @Override
    public ChapObject getChap() {
        return chap;
    }
}
