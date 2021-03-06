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

package com.intel.podm.client;

import com.intel.podm.client.api.ExternalServiceApiActionException;
import com.intel.podm.client.api.ExternalServiceApiReaderConnectionException;
import com.intel.podm.client.api.ExternalServiceApiReaderException;
import com.intel.podm.client.api.WebClient;
import com.intel.podm.client.api.redfish.RedfishClient;
import com.intel.podm.client.api.redfish.response.RedfishConnectionException;
import com.intel.podm.client.api.redfish.response.RedfishEntityResponseBody;
import com.intel.podm.client.api.redfish.response.RedfishException;
import com.intel.podm.client.api.redfish.response.RedfishResponse;
import com.intel.podm.client.api.resources.ExternalServiceResource;

import java.net.URI;
import java.util.Optional;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

import static com.intel.podm.common.concurrent.Tasks.newCurrentThreadTaskScheduler;
import static com.intel.podm.common.utils.Contracts.requiresNonNull;
import static java.util.Optional.empty;
import static java.util.Optional.of;

@SuppressWarnings({"checkstyle:classFanOutComplexity"})
public final class WebClientBasedOnRedfishClient implements WebClient {
    private final RedfishClient redfishClient;

    public WebClientBasedOnRedfishClient(RedfishClient redfishClient) {
        requiresNonNull(redfishClient, "redfishClient");
        this.redfishClient = redfishClient;
    }

    @Override
    public URI getBaseUri() {
        return redfishClient.getServiceRootUri();
    }

    @Override
    public void close() {
        redfishClient.close();
    }

    @Override
    public ExternalServiceResource get(URI uri) throws ExternalServiceApiReaderException {
        requiresNonNull(uri, "uri");

        try {
            RedfishResponse response = redfishClient.get(uri.getPath());

            // TODO: optimistic casting - avoid it
            ExternalServiceResource resource = (ExternalServiceResource) response.getEntity().get();
            resource.setWebClient(this);
            return resource;
        } catch (RedfishConnectionException e) {
            throw new ExternalServiceApiReaderConnectionException("Error while getting resource from the external service", uri, e);
        } catch (RedfishException e) {
            throw new ExternalServiceApiReaderException("Error while getting resource from the external service", uri, e);
        }
    }

    @Override
    public <T> URI post(URI requestUri, T obj) throws ExternalServiceApiActionException {
        requiresNonNull(requestUri, "requestUri");

        try {
            Future<RedfishResponse> responseFuture = obj == null
                    ? redfishClient.postAsync(requestUri.getPath(), newCurrentThreadTaskScheduler())
                    : redfishClient.postAsync(requestUri.getPath(), obj, newCurrentThreadTaskScheduler());

            return responseFuture.get().getLocation().orElse(null);
        } catch (RedfishException e) {
            throw new ExternalServiceApiActionException("Error while posting to the external service", requestUri, e);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new ExternalServiceApiActionException("Error while posting to the external service", requestUri, e);
        } catch (ExecutionException e) {
            throw new ExternalServiceApiActionException("Error while posting to the external service", requestUri, e.getCause());
        }
    }

    @Override
    public <T> Optional<ExternalServiceResource> patch(URI requestUri, T obj) throws ExternalServiceApiActionException {
        requiresNonNull(requestUri, "requestUri");

        try {
            RedfishResponse redfishResponse = redfishClient.patchAsync(requestUri.getPath(), obj, newCurrentThreadTaskScheduler()).get();
            Optional<RedfishEntityResponseBody> optionalResource = redfishResponse.getEntity();
            if (optionalResource.isPresent()) {
                ExternalServiceResource resource = (ExternalServiceResource) optionalResource.get();
                resource.setWebClient(this);
                return of(resource);
            }
            return empty();

        } catch (RedfishException e) {
            throw new ExternalServiceApiActionException("Error while patching to the external service", requestUri, e);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new ExternalServiceApiActionException("Error while patching to the external service", requestUri, e);
        } catch (ExecutionException e) {
            throw new ExternalServiceApiActionException("Error while patching to the external service", requestUri, e.getCause());
        }
    }

    @Override
    public void delete(URI requestUri) throws ExternalServiceApiActionException {
        requiresNonNull(requestUri, "requestUri");

        try {
            Future<RedfishResponse> responseFuture = redfishClient.deleteAsync(requestUri.getPath(), newCurrentThreadTaskScheduler());
            responseFuture.get();
        } catch (RedfishException e) {
            throw new ExternalServiceApiActionException("Error while posting to the external service", requestUri, e);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new ExternalServiceApiActionException("Error while posting to the external service", requestUri, e);
        } catch (ExecutionException e) {
            throw new ExternalServiceApiActionException("Error while posting to the external service", requestUri, e.getCause());
        }
    }
}
