#include <vppinfra/types.h>
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


u64 hash4(ip4_address_t ip_src, 
	  ip4_address_t ip_dst, 
	  u8 protocol, u16 port_src, u16 port_dst) {
  return ip_src.as_u32 ^ ip_dst.as_u32 ^ protocol ^ port_src ^ port_dst;
}


static uword get_flowinfo (vlib_main_t *vm,
                       vlib_node_runtime_t *node,
                       vlib_frame_t *frame)
{
  u32 n_left_from, *from, next_index, *to_next;
  flowtable_main_t *fm = &flowtable_main;
  u32 pkts_processed = 0;

  from = vlib_frame_vector_args(frame);
  n_left_from = frame->n_vectors;
  next_index = node->cached_next_index;

  u64 current_time = clib_cpu_time_now ();

  while(n_left_from > 0) {
    u32 pi0;
    u32 next0;
    u32 n_left_to_next;

    vlib_buffer_t *b0;
    vlib_get_next_frame(vm, node, next_index, to_next, n_left_to_next);

    /* Single loop */
    while (n_left_from > 0 && n_left_to_next > 0) {
	    flow_info_t *flow;
	    uword is_reverse = 0;
	    BVT(clib_bihash_kv) kv;

	    u16 type;
	    next0 = FT_NEXT_ETHERNET_INPUT;
	    pi0 = to_next[0] = from[0];
	    b0 = vlib_get_buffer(vm, pi0);

	    /* Get Flow & copy metadatas into opaque1 or opaque2 */
	    ethernet_header_t *eth0 = (void *) (b0->data + b0->current_data);
            type = clib_net_to_host_u16(eth0->type);
            vlib_buffer_advance (b0, sizeof (ethernet_header_t));
	    ip4_header_t *ip0 = vlib_buffer_get_current(b0);
	    ip4_address_t ip_src = ip0->src_address;
	    ip4_address_t ip_dst = ip0->dst_address;
	    
	    udp_header_t *udp0 = ip4_next_header (ip0);
	    tcp_header_t *tcp0 =  (tcp_header_t *) udp0;
	    u16 port_src = 0, port_dst = 0;

	    if (ip0->protocol == IP_PROTOCOL_UDP ) {
		port_src = udp0->src_port;
		port_dst = udp0->dst_port;
	    } else if (ip0->protocol == IP_PROTOCOL_TCP) {
		port_src = tcp0->ports.src;
		port_dst = tcp0->ports.dst;
	    }

	    (void)type;
	    
	    /* compute 5 tuple key so that 2 half connections 
             * get into the same flow */
	    if (ip_src.as_u32 < ip_dst.as_u32) {
	        kv.key = hash4(ip_src, ip_dst, ip0->protocol, port_src, port_dst);
	    } else {
		is_reverse = 1;
	        kv.key = hash4(ip_dst, ip_src, ip0->protocol, port_dst, port_src);
	    }

	    
	    if (BV(clib_bihash_search) (&fm->flows_ht, &kv, &kv)) {
		pool_get_aligned(fm->flows, flow, CLIB_CACHE_LINE_BYTES);
                /// TODO: we MUST use vec_add_x1 and compare to the signature
		kv.value = pointer_to_uword(flow); 
		BV(clib_bihash_add_del) (&fm->flows_ht, &kv, 1 /* is_add */);
		memset(flow, 0, sizeof(*flow));
		flow->hash = kv.key;
  		vlib_node_increment_counter(vm, flowtable_node.index, 
					    FLOWTABLE_ERROR_FLOW_CREATE, 1 );
	    } else {
	    	flow = uword_to_pointer(kv.value, flow_info_t*);
  		vlib_node_increment_counter(vm, flowtable_node.index, 
					    FLOWTABLE_ERROR_FLOW_HIT, 1 );
	    }
	    if (ip0->protocol == IP_PROTOCOL_TCP) {
		/// TODO: Check TCP State machine
	    }

	    if(is_reverse) {
		flow->packet_stats.reverse++;
	    } else {
		flow->packet_stats.straight++;
	    }
	    flow->last_ts = current_time;

            /* stats */
	    pkts_processed ++;
            vlib_buffer_reset(b0);

	    /* frame mgmt */
	    from++;
	    to_next ++;
	    n_left_from--;
	    n_left_to_next--;

	    if (b0->flags & VLIB_BUFFER_IS_TRACED) {
		flow_trace_t *t = vlib_add_trace(vm, node, b0, sizeof(*t));
		t->hash = kv.key;
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

