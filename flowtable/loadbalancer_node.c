#include <vppinfra/types.h>
#include "flowtable.h"
#include "loadbalancer.h"

vlib_node_registration_t loadbalancer_node;

typedef enum {
  LB_NEXT_DROP,
  LB_NEXT_INTERFACE_OUTPUT,
  LB_NEXT_N_NEXT
} loadbalancer_next_t;


typedef struct {
  u32 sw_if_in;
  u32 sw_if_out;
  u32 next_index;
} lb_trace_t;


static u8 * format_loadbalance(u8 *s, va_list *args) {
  CLIB_UNUSED (vlib_main_t * vm) = va_arg (*args, vlib_main_t *);
  CLIB_UNUSED (vlib_node_t * node) = va_arg (*args, vlib_node_t *);
  lb_trace_t *t = va_arg(*args, lb_trace_t*);

  s = format(s, "LoadBalance - sw_if_in %d, sw_if_out %d, next_index = %d",
			t->sw_if_in, t->sw_if_out, t->next_index);
  return s;
}

static uword load_balance (vlib_main_t *vm,
                       vlib_node_runtime_t *node,
                       vlib_frame_t *frame)
{
  u32 n_left_from, *from, next_index, *to_next;
  loadbalancer_runtime_t *rt = vlib_node_get_runtime_data (vm, loadbalancer_node.index);
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
	    next0 = LB_NEXT_INTERFACE_OUTPUT;
	    pi0 = to_next[0] = from[0];
	    b0 = vlib_get_buffer(vm, pi0);

            /* stats */
	    pkts_processed ++;
	    flow_info_t * fi = *(flow_info_t**) b0->opaque;
	    u32 sw_if_index0 = fi->lb.sw_if_index_current;
	    fi->offloaded = 1;
	    fi->cached_next_node = FT_NEXT_INTERFACE_OUTPUT;
	    fi->lb.sw_if_index_rev = rt->sw_if_index_source;
	    rt->last_target_index++;
	    if(rt->last_target_index >= vec_len(rt->sw_if_target))
		rt->last_target_index = 0;
	    fi->lb.sw_if_index_dst = rt->sw_if_target[rt->last_target_index];

            /* Send pkt back out the RX interface */
	    vnet_buffer(b0)->sw_if_index[VLIB_RX] = sw_if_index0;
            if (sw_if_index0 == rt->sw_if_index_source)
                vnet_buffer(b0)->sw_if_index[VLIB_TX] = fi->lb.sw_if_index_rev;
            else
                vnet_buffer(b0)->sw_if_index[VLIB_TX] = fi->lb.sw_if_index_dst;

	    /* frame mgmt */
	    from++;
	    to_next ++;
	    n_left_from--;
	    n_left_to_next--;

	    if (b0->flags & VLIB_BUFFER_IS_TRACED) {
		lb_trace_t *t = vlib_add_trace(vm, node, b0, sizeof(*t));
		t->sw_if_in = sw_if_index0;
		t->next_index = next0;
		t->sw_if_out = vnet_buffer(b0)->sw_if_index[VLIB_TX];
	    }

	    vlib_validate_buffer_enqueue_x1(vm, node, next_index, to_next, n_left_to_next, pi0, next0);
    }
    vlib_put_next_frame(vm, node, next_index, n_left_to_next);
  }

  vlib_node_increment_counter(vm, loadbalancer_node.index, LOADBALANCER_ERROR_PROCESSED, pkts_processed );

  return frame->n_vectors;
}

static char *loadbalancer_error_strings[] = {
#define _(sym,string) string,
  foreach_loadbalancer_error
#undef _
};

VLIB_REGISTER_NODE(loadbalancer_node) = {
  .function = load_balance,
  .name = "load-balance",
  .vector_size = sizeof(u32),
  .format_trace = format_loadbalance,
  .type = VLIB_NODE_TYPE_INTERNAL,

  .runtime_data_bytes = sizeof(loadbalancer_runtime_t),

  .n_errors = LOADBALANCER_N_ERROR,
  .error_strings = loadbalancer_error_strings,

  .n_next_nodes = LB_NEXT_N_NEXT,
  .next_nodes = {
    [LB_NEXT_DROP] = "error-drop",
    [LB_NEXT_INTERFACE_OUTPUT] = "interface-output",
  }
};


static clib_error_t * loadbalancer_init (vlib_main_t * vm)
{
  clib_error_t * error = 0;
  loadbalancer_main_t *lbm = &loadbalancer_main;
  loadbalancer_runtime_t *rt = vlib_node_get_runtime_data (vm, loadbalancer_node.index);
  rt->lbm = lbm;

  return error;
}

VLIB_INIT_FUNCTION (loadbalancer_init);

