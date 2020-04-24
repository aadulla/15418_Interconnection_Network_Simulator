#include <map>
#include <cassert>
#include <signal.h>

#include "routing_algorithms.h"
#include "network.h"
#include "node.h"
#include "flit.h"

extern NETWORK_TYPE network_type;
extern void* network_info;

uint32_t convert_network_id_to_router_id(void* network_id) {
	uint32_t router_id;

	if (network_type == MESH) {
		Mesh_Info* mesh_info = (Mesh_Info*)network_info;
		Mesh_ID* mesh_id = (Mesh_ID*)network_id;
		router_id = (mesh_id->y * mesh_info->num_cols) + mesh_id->x;
	}

	return router_id;

}

void* convert_router_id_to_network_id(uint32_t router_id) {
	void* network_id;

	if (network_type == MESH) {
		Mesh_Info* mesh_info = (Mesh_Info*)network_info;
		Mesh_ID mesh_id;
		mesh_id.x = (uint32_t)(router_id % mesh_info->num_cols);
		mesh_id.y = (uint32_t)(router_id / mesh_info->num_cols);
		network_id = (void*)&mesh_id;
	}

	return network_id;
}

/* Non-Adaptive Routing Algorithms */

// Route along x dimension first, y dimension second
uint32_t mesh_xy_routing(Flit* flit, 
						 uint32_t curr_router_id,
						 void* curr_network_id,
						 std::map<Flit_Info*, uint32_t, flit_info_comp>* flit_info_to_router_id_map) {

	// convert to Mesh ID
	Mesh_ID* curr_mesh_id = (Mesh_ID*)curr_network_id;

	// construct head flit info for flit
	Flit_Info* flit_info = new Flit_Info;
	flit_info->message_id = flit->message_id;
	flit_info->packet_id = flit->packet_id;

	// if flit is a head flit, then need to insert it and router id to route to
	if (flit->type == HEAD) {
		// get final dest info
		uint32_t final_dest_router_id = ((Head_Flit*)flit)->dest;
		Mesh_ID* final_dest_mesh_id = (Mesh_ID*)convert_router_id_to_network_id(final_dest_router_id);

		// check if flit needs to go to connected processor
		if ((curr_mesh_id->x == final_dest_mesh_id->x) && (curr_mesh_id->y == final_dest_mesh_id->y)) {
			flit_info_to_router_id_map->insert({flit_info, curr_router_id});
			return curr_router_id;
		}

		// mesh id we will be routing flit to
		Mesh_ID next_dest_mesh_id;

		// route x dimension first
		if (curr_mesh_id->x != final_dest_mesh_id->x) {
			// route east
			if (curr_mesh_id->x < final_dest_mesh_id->x) {
				next_dest_mesh_id.x = curr_mesh_id->x + 1;
				next_dest_mesh_id.y = curr_mesh_id->y;
			}
			// route west
			else {
				next_dest_mesh_id.x = curr_mesh_id->x - 1;
				next_dest_mesh_id.y = curr_mesh_id->y;
			}
		}
		// route y dimension second
		else {
			// route north
			if (curr_mesh_id->y > final_dest_mesh_id->y) {
				next_dest_mesh_id.x = curr_mesh_id->x;
				next_dest_mesh_id.y = curr_mesh_id->y - 1;
			}
			// route south
			else {
				next_dest_mesh_id.x = curr_mesh_id->x;
				next_dest_mesh_id.y = curr_mesh_id->y + 1;
			}
		}

		uint32_t next_router_id = convert_network_id_to_router_id(&next_dest_mesh_id);
		flit_info_to_router_id_map->insert({flit_info, next_router_id});
		return next_router_id;
	}

	// if flit is a data flit, then pull the next router id from the map
	else if (flit->type == DATA) {
		auto itr = flit_info_to_router_id_map->find(flit_info);
		if (itr == flit_info_to_router_id_map->end()) raise(SIGTRAP);
		uint32_t next_router_id = itr->second;
		return next_router_id;
	}

	// if flit is a data flit, then a) pull the next router id from the map and b) delete entry in map
	// delete entry because then we route packets individually
	else if (flit->type == TAIL) {
		auto itr = flit_info_to_router_id_map->find(flit_info);
		if (itr == flit_info_to_router_id_map->end()) raise(SIGTRAP);
		uint32_t next_router_id = itr->second;
		flit_info_to_router_id_map->erase(itr);
		return next_router_id;
	}

	else {
		assert(false);
	}

	return (uint32_t)-1;
}


/* Adaptive Routing Algorithms */