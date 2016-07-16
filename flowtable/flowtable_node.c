#include "flowtable.h"

vlib_node_registration_t flowtable_node;

typedef enum {
  FT_NEXT_DROP,
  FT_NEXT_ETHERNET_INPUT,
  FT_NEXT_N_NEXT
} flowtable_next_t;


typedef struct {
  u64 hash;
  u32 sw_if_index;
  u32 next_index;
} flow_trace_t;


static u8 * format_get_flowinfo(u8 *s, va_list *args) {
  CLIB_UNUSED (vlib_main_t * vm) = va_arg (*args, vlib_main_t *);
  CLIB_UNUSED (vlib_node_t * node) = va_arg (*args, vlib_node_t *);
  flow_trace_t *t = va_arg(*args, flow_trace_t*);

  s = format(s, "FlowInfo - sw_if_index %d, hash = 0x%x, next_index = %d",
			t->sw_if_index,
			t->hash, t->next_index);
  return s;
}

static uword get_flowinfo (vlib_main_t *vm,
                       vlib_node_runtime_t *node,
                       vlib_frame_t *frame)
{
  u32 n_left_from, *from, next_index, *to_next;
  u32 pkts_processed = 0;

  from = vlib_frame_vector_args(frame);
  n_left_from = frame->n_vectors;
  next_index = node->cached_next_index;

  while(n_left_from > 0) {
    u32 pi0;
    u32 next0;
    u32 n_left_to_next;

    vlib_buffer_t *b0;
    vlib_get_next_frame(vm, node, next_index, to_next, n_left_to_next);

    /* Single loop */
    while (n_left_from > 0 && n_left_to_next > 0) {
	    next0 = FT_NEXT_ETHERNET_INPUT;
	    pi0 = to_next[0] = from[0];
	    b0 = vlib_get_buffer(vm, pi0);
	    pkts_processed ++;

	    from++;
	    to_next ++;
	    n_left_from--;
	    n_left_to_next--;

	    if (b0->flags & VLIB_BUFFER_IS_TRACED) {
		flow_trace_t *t = vlib_add_trace(vm, node, b0, sizeof(*t));
		t->hash = 42;
		t->sw_if_index = vnet_buffer(b0)->sw_if_index[VLIB_RX];
		t->next_index = next0;
	    }

	    vlib_validate_buffer_enqueue_x1(vm, node, next_index, to_next, n_left_to_next, pi0, next0);
    }
    vlib_put_next_frame(vm, node, next_index, n_left_to_next);
  }

  vlib_node_increment_counter(vm, flowtable_node.index, FLOWTABLE_ERROR_THRU, pkts_processed );

  return frame->n_vectors;
}

static char *flowtable_error_strings[] = {
#define _(sym,string) string,
  foreach_flowtable_error
#undef _
};

VLIB_REGISTER_NODE(flowtable_node) = {
  .function = get_flowinfo,
  .name = "flow-get",
  .vector_size = sizeof(u32),
  .format_trace = format_get_flowinfo,
  .type = VLIB_NODE_TYPE_INTERNAL,
  .n_errors = FLOWTABLE_N_ERROR,
  .error_strings = flowtable_error_strings,
  .n_next_nodes = FT_NEXT_N_NEXT,
  .next_nodes = {
    [FT_NEXT_DROP] = "error-drop",
    [FT_NEXT_ETHERNET_INPUT] = "ethernet-input",
  }
};

