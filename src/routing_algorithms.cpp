#include <map>
#include <cassert>
#include <signal.h>
#include <stdlib.h>
#include <algorithm>

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

	if (network_type == MESH) {
		Mesh_Info* mesh_info = (Mesh_Info*)network_info;
		Mesh_ID* mesh_id = new Mesh_ID;
		mesh_id->x = (uint32_t)(router_id % mesh_info->num_cols);
		mesh_id->y = (uint32_t)(router_id / mesh_info->num_cols);
		return (void*)mesh_id;
	}

	else assert(false);
}

int routing_cache_lookup(Flit* flit, 
						 uint32_t curr_router_id,
						 void* curr_network_id,
						 Flit_Info_To_Router_ID_Cache* flit_info_to_router_id_cache) {

	// construct head flit info for flit
	Flit_Info* flit_info = new Flit_Info;
	flit_info->message_id = flit->message_id;
	flit_info->packet_id = flit->packet_id;

	// if flit is a head flit, then need to insert it and router id to route to
	if (flit->type == HEAD) {

		if (network_type == MESH) {
			Mesh_ID* curr_mesh_id = (Mesh_ID*)curr_network_id;
			uint32_t final_dest_router_id = ((Head_Flit*)flit)->dest;
			Mesh_ID* final_dest_mesh_id = (Mesh_ID*)convert_router_id_to_network_id(final_dest_router_id);

			// check if flit needs to go to connected processor
			if ((curr_mesh_id->x == final_dest_mesh_id->x) && (curr_mesh_id->y == final_dest_mesh_id->y)) {
				flit_info_to_router_id_cache->insert(flit_info, curr_router_id);
				return (int)curr_router_id;
			}

			// check if this is a retry because head did not go through in previous transmission
			// in this case, erase previous entry and create new entry whic is reflective of present network state
			else if (flit_info_to_router_id_cache->is_in(flit_info)) {
				flit_info_to_router_id_cache->erase(flit_info);
				return -1;
			}

			else return -1;
		}

		// add more network types here
		else assert(false);
	} 

	// if flit is a data flit, then pull the next router id from the map
	else if (flit->type == DATA) {
		uint32_t next_router_id = flit_info_to_router_id_cache->find(flit_info);
		return (int)next_router_id;
	}

	// if flit is a data flit, then a) pull the next router id from the map and b) delete entry in map
	// delete entry because then we route packets individually
	else if (flit->type == TAIL) {
		uint32_t next_router_id = flit_info_to_router_id_cache->find(flit_info);
		return (int)next_router_id;
	}

	else assert(false);
	return -1;
}

bool is_unreserved_buffer(void* network_id, std::map<Router*, std::vector<IO_Channel*>*>* neighbor_to_io_channels_map) {
	uint32_t next_router_id = convert_network_id_to_router_id(network_id);
	for (auto itr_router=neighbor_to_io_channels_map->begin(); itr_router != neighbor_to_io_channels_map->end(); itr_router++) {
		Router* router = itr_router->first;
		if (router->node_id == next_router_id) {
			std::vector<IO_Channel*>* io_channel_vec = itr_router->second;
			for (auto itr_channel=io_channel_vec->begin(); itr_channel != io_channel_vec->end(); itr_channel++) {
				Channel* output_channel = (*itr_channel)->output_channel;
				if (output_channel->is_dest_buffer_unreserved()) return true;
			}
		}
	}
	return false;
}


/* Non-Adaptive Routing Algorithms */

// Route along x dimension first, y dimension second
uint32_t mesh_xy_routing(Flit* flit, 
						 uint32_t curr_router_id,
						 void* curr_network_id,
						 Flit_Info_To_Router_ID_Cache* flit_info_to_router_id_cache,
						 std::map<Router*, std::vector<IO_Channel*>*>* neighbor_to_io_channels_map) {

	int cached_next_router_id = routing_cache_lookup(flit, curr_router_id, curr_network_id, flit_info_to_router_id_cache);
	if (cached_next_router_id != -1) return (uint32_t)cached_next_router_id;

	assert(flit->type == HEAD);

	// convert to Mesh ID
	Mesh_ID* curr_mesh_id = (Mesh_ID*)curr_network_id;
	uint32_t final_dest_router_id = ((Head_Flit*)flit)->dest;
	Mesh_ID* final_dest_mesh_id = (Mesh_ID*)convert_router_id_to_network_id(final_dest_router_id);

	// construct head flit info for flit
	Flit_Info* flit_info = new Flit_Info;
	flit_info->message_id = flit->message_id;
	flit_info->packet_id = flit->packet_id;

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

	uint32_t next_router_id = convert_network_id_to_router_id((void*)&next_dest_mesh_id);
	flit_info_to_router_id_cache->insert(flit_info, next_router_id);
	return next_router_id;

}

// Route along y dimension first, x dimension second
uint32_t mesh_yx_routing(Flit* flit, 
						 uint32_t curr_router_id,
						 void* curr_network_id,
						 Flit_Info_To_Router_ID_Cache* flit_info_to_router_id_cache,
						 std::map<Router*, std::vector<IO_Channel*>*>* neighbor_to_io_channels_map) {

	int cached_next_router_id = routing_cache_lookup(flit, curr_router_id, curr_network_id, flit_info_to_router_id_cache);
	if (cached_next_router_id != -1) return (uint32_t)cached_next_router_id;

	assert(flit->type == HEAD);

	// convert to Mesh ID
	Mesh_ID* curr_mesh_id = (Mesh_ID*)curr_network_id;
	uint32_t final_dest_router_id = ((Head_Flit*)flit)->dest;
	Mesh_ID* final_dest_mesh_id = (Mesh_ID*)convert_router_id_to_network_id(final_dest_router_id);

	// construct head flit info for flit
	Flit_Info* flit_info = new Flit_Info;
	flit_info->message_id = flit->message_id;
	flit_info->packet_id = flit->packet_id;

	Mesh_ID next_dest_mesh_id;

	// route y dimension first
	if (curr_mesh_id->y != final_dest_mesh_id->y) {
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
	// route x dimension second
	else {
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

	uint32_t next_router_id = convert_network_id_to_router_id((void*)&next_dest_mesh_id);
	flit_info_to_router_id_cache->insert(flit_info, next_router_id);
	return next_router_id;
}


/* Adaptive Routing Algorithms */

uint32_t mesh_adaptive_routing(Flit* flit, 
						 uint32_t curr_router_id,
						 void* curr_network_id,
						 Flit_Info_To_Router_ID_Cache* flit_info_to_router_id_cache,
						 std::map<Router*, std::vector<IO_Channel*>*>* neighbor_to_io_channels_map) {

	int cached_next_router_id = routing_cache_lookup(flit, curr_router_id, curr_network_id, flit_info_to_router_id_cache);
	if (cached_next_router_id != -1) return (uint32_t)cached_next_router_id;

	assert(flit->type == HEAD);

	// convert to Mesh ID
	Mesh_ID* curr_mesh_id = (Mesh_ID*)curr_network_id;
	uint32_t final_dest_router_id = ((Head_Flit*)flit)->dest;
	Mesh_ID* final_dest_mesh_id = (Mesh_ID*)convert_router_id_to_network_id(final_dest_router_id);

	// construct head flit info for flit
	Flit_Info* flit_info = new Flit_Info;
	flit_info->message_id = flit->message_id;
	flit_info->packet_id = flit->packet_id;

	bool is_x_valid_move = false;
	bool is_y_valid_move = false;

	Mesh_ID x_next_dest_mesh_id;
	Mesh_ID y_next_dest_mesh_id;

	if (curr_mesh_id->x != final_dest_mesh_id->x) {
		// route east
		if (curr_mesh_id->x < final_dest_mesh_id->x) {
			x_next_dest_mesh_id.x = curr_mesh_id->x + 1;
			x_next_dest_mesh_id.y = curr_mesh_id->y;
		}
		// route west
		else {
			x_next_dest_mesh_id.x = curr_mesh_id->x - 1;
			x_next_dest_mesh_id.y = curr_mesh_id->y;
		}
		is_x_valid_move = true;
	}

	if (curr_mesh_id->y != final_dest_mesh_id->y) {
		// route north
		if (curr_mesh_id->y > final_dest_mesh_id->y) {
			y_next_dest_mesh_id.x = curr_mesh_id->x;
			y_next_dest_mesh_id.y = curr_mesh_id->y - 1;
		}
		// route south
		else {
			y_next_dest_mesh_id.x = curr_mesh_id->x;
			y_next_dest_mesh_id.y = curr_mesh_id->y + 1;
		}
		is_y_valid_move = true;
	}

	uint32_t x_next_router_id;
	if (is_x_valid_move) x_next_router_id = convert_network_id_to_router_id((void*)&x_next_dest_mesh_id);
	uint32_t y_next_router_id;
	if (is_y_valid_move) y_next_router_id = convert_network_id_to_router_id((void*)&y_next_dest_mesh_id);

	uint32_t next_router_id = (uint32_t)-1;

	// if both x and y moves are valid, then choose the one with least traffic at next dest
	if (is_x_valid_move && is_y_valid_move) {

		bool is_x_unreserved_buffer = is_unreserved_buffer((void*)&x_next_dest_mesh_id, neighbor_to_io_channels_map);
		bool is_y_unreserved_buffer = is_unreserved_buffer((void*)&y_next_dest_mesh_id, neighbor_to_io_channels_map);

		// if both have unreserved buffers, then randomly pick one
		if (is_x_unreserved_buffer && is_y_unreserved_buffer) {
			next_router_id = x_next_router_id;
			// // choose x
			// if ((((float)rand()) / RAND_MAX) < 0.5) next_router_id = x_next_router_id;
			// // choose y
			// else next_router_id = y_next_router_id;
		}
		// only x has unreserved
		else if (is_x_unreserved_buffer) next_router_id = x_next_router_id;
		// only y has unreserved
		else if (is_y_unreserved_buffer) next_router_id = y_next_router_id;
		// neither have unreserved, then randomly pick one
		else {
			next_router_id = y_next_router_id;
			// // choose x
			// if ((((float)rand()) / RAND_MAX) < 0.5) next_router_id = x_next_router_id;
			// // choose y
			// else next_router_id = y_next_router_id;
		}
	}
	// only x is valid move
	else if (is_x_valid_move) next_router_id = x_next_router_id;
	// only y is valid move
	else if (is_y_valid_move) next_router_id = y_next_router_id;
	//should never come here
	else assert(false);

	assert(next_router_id != (uint32_t)-1);

	flit_info_to_router_id_cache->insert(flit_info, next_router_id);
	// raise(SIGTRAP);
	return next_router_id;
}