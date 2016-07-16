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


#define foreach_flowtable_error				\
 _(THRU, "Flowtable packets gone thru")

typedef enum {
#define _(sym,str) FLOWTABLE_ERROR_##sym,
   foreach_flowtable_error
#undef _
   FLOWTABLE_N_ERROR
 } flowtable_error_t;

typedef struct {
  /* monotonic increasing flow counter */
  u64 hash;
  u64 flow_id;

  u16 sig_len;
  u8 signature[16];

  u8 flow_data[0];
} flow_info_t;

typedef struct {
  /* pool of flows */
  flow_info_t *flows;

  /* convenience */
  vlib_main_t *vlib_main;
  vnet_main_t *vnet_main;

  u32 ethernet_input_next_index;
} flowtable_main_t;

flowtable_main_t flowtable_main;

extern vlib_node_registration_t flowtable_node;
