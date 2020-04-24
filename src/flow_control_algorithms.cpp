#include <map>
#include <cassert>
#include <signal.h>

#include "routing_algorithms.h"
#include "node.h"
#include "buffer.h"
#include "flit.h"

/* Packet-Based Flow Control */

void packet_tx_store_forward (void* router_ptr) {
	Router* router = (Router*)router_ptr;

	// loop through all input channels
	for (auto itr=router->input_channel_to_buffers_map->begin(); itr != router->input_channel_to_buffers_map->end(); itr++) {

		// loop through all buffers
		Buffer** buffers = itr->second;
		for (uint32_t i=0; i < router->num_virtual_channels; i++) {
			Buffer* buffer = buffers[i];
			if (buffer->is_empty()) continue;

			Flit* flit = buffer->peek_flit();

			// determine where to route next flit in buffer to
			uint32_t next_dest_router_id = (*(router->routing_func))(flit, 
																   	 router->node_id, 
																   	 router->network_id,
																   	 router->flit_info_to_router_id_map);

			bool is_proposed = false;
			std::vector<IO_Channel*>* io_channel_vec = router->get_io_channel_vec(next_dest_router_id);

			// check if channel is already locked for this flit
			for (auto itr_io_channel=io_channel_vec->begin(); itr_io_channel != io_channel_vec->end(); itr_io_channel++) {
				Channel* output_channel = (*itr_io_channel)->output_channel;
				bool is_locked_for_flit = output_channel->is_locked_for_flit(flit);
				bool is_open_for_transmission = output_channel->is_open_for_transmission();
				if (is_locked_for_flit && is_open_for_transmission) {
					output_channel->propose_transmission(buffer);
					// if transmitting a tail flit, unlock channel 
					if (flit->type == TAIL) output_channel->unlock();
					is_proposed = true;
					break;
				}
			}

			// check if the flit has already been proposed for tranmission on an ouput channel
			if (is_proposed == false) {
				// check if channel is open for transmission
				// might need to add in more logic for arbitrating this because may choose the same buffer each time
				for (auto itr_io_channel=io_channel_vec->begin(); itr_io_channel != io_channel_vec->end(); itr_io_channel++) {
					Channel* output_channel = (*itr_io_channel)->output_channel;
					bool is_unlocked_for_flit = output_channel->is_unlocked_for_flit(flit);
					bool is_open_for_transmission = output_channel->is_open_for_transmission();
					if (is_unlocked_for_flit && is_open_for_transmission) {
						output_channel->propose_transmission(buffer);
						// if transmitting a head flit, lock channel (always true)
						if (flit->type == HEAD) output_channel->lock();
						else {
							raise(SIGTRAP);
							assert(false);
						}
						break;
					}
				}
			}
		}
	}
}

void packet_rx_store_forward (void* router_ptr) {
	Router* router = (Router*)router_ptr;

	// loop through all input channels
	for (auto itr=router->input_channel_to_buffers_map->begin(); itr != router->input_channel_to_buffers_map->end(); itr++) {
		Channel* input_channel = itr->first;
		// check if channel has a pending transmission
		if (input_channel->is_open_for_transmission()) continue;

		uint32_t proposed_message_id = input_channel->transmission_state->message_id;
		uint32_t proposed_packet_id = input_channel->transmission_state->packet_id;

		Buffer** buffers = itr->second;
		bool is_executed = false;

		// check if there is a buffer already holding this flit info 
		for (uint32_t i=0; i < router->num_virtual_channels; i++) {
			Buffer* buffer = buffers[i];
			// only look at buffers which are not full
			bool is_not_full = buffer->is_not_full();
			if (is_not_full) {
				auto itr_buffer = router->internal_info_summary->buffer_to_flit_info_set_map->find(buffer);
				assert(itr_buffer != router->internal_info_summary->buffer_to_flit_info_set_map->end());
				std::set<Flit_Info*, flit_info_comp>* flit_info_set = itr_buffer->second;
				Flit_Info proposed_flit_info;
				proposed_flit_info.message_id = proposed_message_id;
				proposed_flit_info.packet_id = proposed_packet_id;
				if (flit_info_set->find(&proposed_flit_info) != flit_info_set->end()) {
					FLIT_TYPE flit_type = input_channel->execute_transmission(buffer);
					if (flit_type == TAIL) input_channel->reset_transmission_state();
					is_executed = true;
					break;
				}
			}
		}

		// check if flit has already been pulled in by executing transmission
		if (is_executed == false) {
			// check if there is an empty buffer
			for (uint32_t i=0; i < router->num_virtual_channels; i++) {
				Buffer* buffer = buffers[i];
				// only look at buffers which are not full
				bool is_empty = buffer->is_empty();
				if (is_empty) {
					FLIT_TYPE flit_type = input_channel->execute_transmission(buffer);
					if (flit_type == TAIL) input_channel->reset_transmission_state();
					is_executed = true;
					break;
				}
			}
		}

		if (is_executed == false) input_channel->fail_transmission();
	}
}