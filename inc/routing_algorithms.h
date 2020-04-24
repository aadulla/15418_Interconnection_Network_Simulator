#ifndef ROUTING_ALGORITHMS_H
#define ROUTING_ALGORITHMS_H

#include <stdio.h>
#include <map>

#include "flit.h"

typedef struct _Flit_Info {
	uint32_t message_id;
	uint32_t packet_id;
} Flit_Info;

struct flit_info_comp {
	bool operator()(const Flit_Info* flit_info_1, const Flit_Info* flit_info_2) {
		if (flit_info_1->message_id < flit_info_2->message_id) return true;
		if (flit_info_1->message_id == flit_info_2->message_id) {
			if (flit_info_1->packet_id < flit_info_2->packet_id) return true;
		}
		return false;
	}
};

typedef uint32_t (*Routing_Func)(Flit*, uint32_t, void*, std::map<Flit_Info*, uint32_t, flit_info_comp>*);

uint32_t convert_topology_id_to_router_id(void* network_id);
void* convert_router_id_to_topology_id(uint32_t router_id);

/* Non-Adaptive Routing Algorithms */
uint32_t mesh_xy_routing(Flit* flit, uint32_t curr_router_id, void* curr_network_id, std::map<Flit_Info*, uint32_t, flit_info_comp>* flit_info_to_router_id_map);

/* Adaptive Routing Algorithms */

#endif /* ROUTING_ALGORITHMS_H */