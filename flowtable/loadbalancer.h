/*---------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------
 */
#include <stdbool.h>
#include <vppinfra/error.h>
#include <vnet/vnet.h>
#include <vnet/ip/ip.h>


#define foreach_loadbalancer_error				\
 _(PROCESSED, "Loadbalancer packets gone thru") \


typedef enum {
#define _(sym,str) LOADBALANCER_ERROR_##sym,
   foreach_loadbalancer_error
#undef _
   LOADBALANCER_N_ERROR
 } loadbalancer_error_t;

typedef struct {

  /* convenience */
  vlib_main_t *vlib_main;
  vnet_main_t *vnet_main;

} loadbalancer_main_t;

typedef struct {
  loadbalancer_main_t *lbm;
  
  u32 sw_if_index_source;
  u32 *sw_if_target;

  u32 last_target_index;

}loadbalancer_runtime_t;

loadbalancer_main_t loadbalancer_main;

extern vlib_node_registration_t loadbalancer_node;
