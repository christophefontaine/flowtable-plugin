/*
 * Copyright (c) 2016 Qosmos and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vnet/plugin/plugin.h>
#include "flowtable.h"
#include "loadbalancer.h"


/* 
 * This routine exists to convince the vlib plugin framework that
 * we haven't accidentally copied a random .dll into the plugin directory.
 *
 * Also collects global variable pointers passed from the vpp engine
 */
clib_error_t * 
vlib_plugin_register (vlib_main_t * vm, vnet_plugin_handoff_t * h,
                      int from_early_init)
{
  clib_error_t * error = 0;
  flowtable_main_t *fm = &flowtable_main;

  fm->vnet_main = vnet_get_main();
  fm->vlib_main = vm;

  loadbalancer_main_t *lbm = &loadbalancer_main;

  lbm->vnet_main = vnet_get_main();
  lbm->vlib_main = vm;

  return error;
}

static clib_error_t * flowtable_init (vlib_main_t * vm)
{
  clib_error_t * error = 0;
  flowtable_main_t *fm = &flowtable_main;

  /* TODO get flow count from configuration */

  flow_info_t *flow;
  pool_get_aligned(fm->flows, flow, CLIB_CACHE_LINE_BYTES);
  pool_put(fm->flows, flow);

  BV(clib_bihash_init) (&fm->flows_ht, "flow hash table",
                        FM_NUM_BUCKETS, FM_MEMORY_SIZE);
  return error;
}

VLIB_INIT_FUNCTION (flowtable_init);
