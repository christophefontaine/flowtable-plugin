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

#include "flowtable.h"
#include "loadbalancer.h"
#include <vnet/plugin/plugin.h>

void vl_api_rpc_call_main_thread (void *fp, u8 * data, u32 data_length);


int loadbalancer_set_targets(loadbalancer_main_t* lb, u32 sw_if_index_source, u32 * sw_if_index_targets) {
  u32 *sw_if_index;
  vec_foreach(sw_if_index, sw_if_index_targets) {
  }
  return 0;
}

static clib_error_t *
set_lb_target_command_fn (vlib_main_t * vm,
                                     unformat_input_t * input,
                                     vlib_cli_command_t * cmd)
{
  flowtable_main_t * fm = &flowtable_main;
  loadbalancer_main_t * lb = &loadbalancer_main;
  u32 sw_if_index = ~0;
  u32 sw_if_index_source = ~0;
  u32 *sw_if_index_targets = NULL;
  u32 *v;
  int source = 1;
  vec_validate(sw_if_index_targets, 10);

  int rv;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
    if (unformat (input, "to")) 
       source = 0;
    else if (unformat (input, "%U", unformat_vnet_sw_interface,
                       fm->vnet_main, &sw_if_index)) {
	if (source)
		sw_if_index_source = sw_if_index;
	else /* Add target to the vector */
		vec_add1(sw_if_index_targets, sw_if_index);
    }
    else
      break;
  }

  if (sw_if_index == ~0)
    return clib_error_return (0, "No SOURCE Interface specified");

  if(vec_len(sw_if_index_targets) <= 0) {
    return clib_error_return (0, "No TARGET Interface specified");
  }
  
  rv = flowtable_enable_disable(fm, sw_if_index_source, 1);
  vec_foreach(v, sw_if_index_targets) {
    rv = flowtable_enable_disable(fm, *v, 1);
  }
  switch(rv) {
    case 0: break;
    case VNET_API_ERROR_INVALID_SW_IF_INDEX: return clib_error_return(0, "Invalid interface");
    case VNET_API_ERROR_UNIMPLEMENTED: return clib_error_return (0, "Device driver doesn't support redirection"); break;
    default: return clib_error_return (0, "flowtable_enable_disable returned %d", rv); break;
  }

  rv = loadbalancer_set_targets(lb, sw_if_index_source, sw_if_index_targets);
  switch(rv) {
    case 0: break;
    default: return clib_error_return (0, "set interface loadbalancer returned %d", rv); break;
  }

  return 0;
}


VLIB_CLI_COMMAND(set_interface_loadbalanced_command) = {
  .path = "set interface loadbalanced",
  .short_help = 
  "set interface loadbalanced <interface> to <interfaces-list>",
  .function = set_lb_target_command_fn,
};

