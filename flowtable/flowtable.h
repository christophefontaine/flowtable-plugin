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
#include <vppinfra/pool.h>
#include <vppinfra/bihash_8_8.h>


#define foreach_flowtable_error				\
 _(THRU, "Flowtable packets gone thru") \
 _(FLOW_CREATE, "Flowtable packets which created a new flow") \
 _(FLOW_HIT, "Flowtable packets with an existing flow") 

typedef enum {
#define _(sym,str) FLOWTABLE_ERROR_##sym,
   foreach_flowtable_error
#undef _
   FLOWTABLE_N_ERROR
 } flowtable_error_t;

typedef struct {
  /* monotonic increasing flow counter */
  u64 hash;

  u32 cached_next_node;
  u16 offloaded;

  u16 sig_len;
  union {
      struct {
	ip6_address_t src, dst;
	u8 proto;
	u16 port_src, port_dst;
      }ip6;
      struct {
	ip4_address_t src, dst;
	u8 proto;
	u16 port_src, port_dst;
      }ip4;
      u8 data[32];
  } signature;

  u64 last_ts;
  struct {
    u32 straight;
    u32 reverse;
  } packet_stats;

  /* the following union will be copied to opaque1 
   * it MUST be less or equal CLIB_CACHE_LINE_BYTES */
  union {
	struct {
	    u32 SYN:1;
	    u32 SYN_ACK:1;
	    u32 SYN_ACK_ACK:1;

	    u32 FIN:1;
	    u32 FIN_ACK:1;
	    u32 FIN_ACK_ACK:1;
	
	    u32 last_seq_number, last_ack_number;
	};
        u8 flow_data[CLIB_CACHE_LINE_BYTES];
  };
} flow_info_t;

typedef struct {
  /* pool of flows */
  flow_info_t *flows;
  /* hashtable */
  BVT(clib_bihash) flows_ht;
  

  /* convenience */
  vlib_main_t *vlib_main;
  vnet_main_t *vnet_main;

  u32 ethernet_input_next_index;
} flowtable_main_t;

flowtable_main_t flowtable_main;

#define FM_NUM_BUCKETS 4
#define FM_MEMORY_SIZE (256<<16)

extern vlib_node_registration_t flowtable_node;

int flowtable_enable_disable (flowtable_main_t *fm,
                              u32 sw_if_index, int enable_disable);
