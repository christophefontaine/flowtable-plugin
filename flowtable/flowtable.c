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
#include <vnet/plugin/plugin.h>


#if 0
static clib_error_t *
sixrd_del_domain_command_fn (vlib_main_t *vm,
			   unformat_input_t *input,
			   vlib_cli_command_t *cmd)
{
  unformat_input_t _line_input, *line_input = &_line_input;
  u32 num_m_args = 0;
  u32 sixrd_domain_index;

  /* Get a line of input. */
  if (! unformat_user(input, unformat_line_input, line_input))
    return 0;
 
  while (unformat_check_input(line_input) != UNFORMAT_END_OF_INPUT) {
    if (unformat(line_input, "index %d", &sixrd_domain_index))
      num_m_args++;
    else
      return clib_error_return(0, "unknown input `%U'",
				format_unformat_error, input);
  }
  unformat_free(line_input);

  if (num_m_args != 1)
    return clib_error_return(0, "mandatory argument(s) missing");

  sixrd_delete_domain(sixrd_domain_index);

  return 0;
}


VLIB_CLI_COMMAND(sixrd_del_command, static) = {
  .path = "sixrd del domain",
  .short_help = 
  "sixrd del domain index <domain>",
  .function = sixrd_del_domain_command_fn,
};

#endif


int flowtable_enable_disable (flowtable_main_t *fm,
			      u32 sw_if_index, int enable_disable)
{
  u32 node_index = enable_disable ? flowtable_node.index : ~0;

  return vnet_hw_interface_rx_redirect_to_node (fm->vnet_main, 
                                                sw_if_index, 
                                                node_index);
}

static clib_error_t *
flowtable_enable_disable_command_fn (vlib_main_t * vm,
                                     unformat_input_t * input,
                                     vlib_cli_command_t * cmd)
{
  flowtable_main_t * fm = &flowtable_main;
  u32 sw_if_index = ~0;
  int enable_disable = 1;

  int rv;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
    if (unformat (input, "disable")) 
       enable_disable = 0;
    else if (unformat (input, "%U", unformat_vnet_sw_interface,
                       fm->vnet_main, &sw_if_index));
    else
      break;
  }

  if (sw_if_index == ~0)
    return clib_error_return (0, "No Interface specified");
  
  rv = flowtable_enable_disable(fm, sw_if_index, enable_disable);
  switch(rv) {
    case 0: break;
    case VNET_API_ERROR_INVALID_SW_IF_INDEX: return clib_error_return(0, "Invalid interface");
    case VNET_API_ERROR_UNIMPLEMENTED: return clib_error_return (0, "Device driver doesn't support redirection"); break;
    default: return clib_error_return (0, "flowtable_enable_disable returned %d", rv); break;
  }

  return 0;
}


VLIB_CLI_COMMAND(flowtable_interface_enable_disable_command) = {
  .path = "flowtable",
  .short_help = 
  "flowtable <interface> [disable]",
  .function = flowtable_enable_disable_command_fn,
};


/*
VLIB_CLI_COMMAND(flowtable_enable_disable_command, static) = {
  .path = "flowtable node",
  .short_help = 
  "flowtable node <node> [disable]",
  .function = flowtable_enable_disable_command_fn,
};
*/


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
  flowtable_main_t *mm = &flowtable_main;

  mm->vnet_main = vnet_get_main();
  mm->vlib_main = vm;

  return error;
}

static clib_error_t * flowtable_init (vlib_main_t * vm)
{
  clib_error_t * error = 0; 
#if 0
  flowtable_main_t *fm = &flowtable_main;

  vlib_node_t * l2_input_node = vlib_get_node_by_name(vm, (u8 *)"l2-input");
  ASSERT(l2_input_node);

  fm->ethernet_input_next_index = vlib_node_add_next(vm, 
                                          l2_input_node->index,
                                          flowtable_node.index);
#endif
  return error;
}

VLIB_INIT_FUNCTION (flowtable_init);
