/**
 * Copyright (c)  2015-2017 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __ASSETD_NMAP_H__
#define __ASSETD_NMAP_H__

#include <stdio.h>
#include "libutils/rmm.h"
#include "libmemdb/memdb.h"

void nmap_add(char* uuid, memdb_integer node_id);
void nmap_remove_by_node_id(memdb_integer node_id);
int nmap_get_node_id_by_uuid(memdb_integer *node_id, char *uuid);

#endif
