#include <stdint.h>
#include <stdio.h>
#include <cassert>
#include <map>
#include <vector>
#include <string>
#include <signal.h>
#include <atomic>

#include "routing_algorithms.h"
#include "node.h"
#include "channel.h"
#include "buffer.h"
#include "message.h"

extern uint32_t num_data_flits_per_packet;
extern uint32_t global_clock;
extern Message_Transmission_Info** global_message_transmission_info;

extern std::atomic<int> flits_in_network;

Internal_Info_Summary::Internal_Info_Summary () {
	this->message_id_to_packet_id_set_map = new std::map<uint32_t, std::set<uint32_t>*>;
	this->buffer_to_flit_info_set_map = new std::map<Buffer*, std::set<Flit_Info*, flit_info_comp>*>;
	this->buffer_space_occupied = 0;
	this->buffer_space_total = 0;
	this->num_failed_transmissions = 0;
}

void Internal_Info_Summary::init_buffer_in_map (Buffer* buffer) {
	std::set<Flit_Info*, flit_info_comp>* flit_info_set = new std::set<Flit_Info*, flit_info_comp>;
	this->buffer_to_flit_info_set_map->insert({buffer, flit_info_set});
}

void Internal_Info_Summary::insert (Buffer* buffer, uint32_t message_id, uint32_t packet_id) {
	auto itr_message = this->message_id_to_packet_id_set_map->find(message_id);
	// first check if message id is already in map
	if (itr_message != this->message_id_to_packet_id_set_map->end()) {
		std::set<uint32_t>* packet_id_set = itr_message->second;
		// second check if packet is not already in set
		if (packet_id_set->find(packet_id) == packet_id_set->end()) {
			packet_id_set->insert(packet_id);

			// add flit info to map so that buffer can keep track of its contents
			Flit_Info* flit_info = new Flit_Info;
			flit_info->message_id = message_id;
			flit_info->packet_id = packet_id;
			auto itr_buffer = this->buffer_to_flit_info_set_map->find(buffer);
			assert(itr_buffer != this->buffer_to_flit_info_set_map->end());
			std::set<Flit_Info*, flit_info_comp>* flit_info_set = itr_buffer->second;
			flit_info_set->insert(flit_info);
		}
		return;
	}

 	// message id was not in map, so make new entry in map
 	std::set<uint32_t>* packet_id_set = new std::set<uint32_t>;
 	packet_id_set->insert(packet_id);
 	this->message_id_to_packet_id_set_map->insert({message_id, packet_id_set});

 	// message id was not in map, so add to flit info vec
 	Flit_Info* flit_info = new Flit_Info;
	flit_info->message_id = message_id;
	flit_info->packet_id = packet_id;
	auto itr_buffer = this->buffer_to_flit_info_set_map->find(buffer);
	assert(itr_buffer != this->buffer_to_flit_info_set_map->end());
	std::set<Flit_Info*, flit_info_comp>* flit_info_set = itr_buffer->second;
	flit_info_set->insert(flit_info);
}

void Internal_Info_Summary::clear () {
	this->message_id_to_packet_id_set_map->clear();
	for (auto itr=this->buffer_to_flit_info_set_map->begin(); itr != this->buffer_to_flit_info_set_map->end(); itr++) {
		itr->second->clear();
	}
	this->buffer_space_occupied = 0;
	this->buffer_space_total = 0;
	this->num_failed_transmissions = 0;
}

Node::Node (uint32_t node_id, void* network_id, uint32_t num_channels, uint32_t num_neighbors, uint32_t max_buffer_capacity, NODE_TYPE type) {
	this->node_id = node_id;
	this->network_id = network_id;
	this->num_channels = num_channels;
	this->num_neighbors = num_neighbors;
	this->max_buffer_capacity = max_buffer_capacity;
	this->type = type;
}

Processor::Processor (uint32_t node_id, 
					  void* network_id, 
					  uint32_t num_channels, 
					  uint32_t num_neighbors, 
					  uint32_t max_buffer_capacity) :
Node(node_id, network_id, num_channels, num_neighbors, max_buffer_capacity, PROCESSOR) {
	this->injection_buffer = new Buffer(this->max_buffer_capacity);
	this->router_buffer = new Buffer(this->max_buffer_capacity);
	this->num_flits_transmitted	= 0;
	this->num_flits_received = 0;
	this->transmitted_messages_vec = new std::vector<uint32_t>;
	this->received_messages_vec = new std::vector<uint32_t>;
	this->receive_message_flag = false;
	this->transmit_message_flag = false;
}

void Processor::init_tx_message_vec(std::vector<Message*>* tx_message_vec) {
	this->tx_message_vec = tx_message_vec;
}

void Processor::init_rx_message_map(std::map<uint32_t, int>* rx_message_id_to_num_flits_map) {
	this->rx_message_id_to_num_flits_map = rx_message_id_to_num_flits_map;
}

void Processor::init_connection (Node* node, Channel* input_channel, Channel* output_channel) {
	if (node->type == PROCESSOR_ROUTER) this->init_router_connection((Router*)node, input_channel, output_channel);
	// should never come here
	else assert(false);
}

void Processor::init_router_connection (Router* router, Channel* input_channel, Channel* output_channel) {
	this->router = router;
	this->router_input_channel = input_channel;
	this->router_output_channel = output_channel;
}

void Processor::inject_message(Message* message) {
	for (uint32_t i=0; i < message->num_packets; i++) {
		Packet* packet = message->packet_lst[i];
		Head_Flit* head_flit = packet->head;
		this->injection_buffer->insert_flit(head_flit);
		this->num_flits_transmitted++;
		for (uint32_t j=0; j < num_data_flits_per_packet; j++) {
			Flit* data_flit = packet->payload[j];
			this->injection_buffer->insert_flit(data_flit);
			this->num_flits_transmitted++;
		}
		Tail_Flit* tail_flit = packet->tail;
		this->injection_buffer->insert_flit(tail_flit);
		this->num_flits_transmitted++;
	}
}

void Processor::tx () {
	// if injection buffer is empty, add new message to transmit
	if (this->injection_buffer->is_empty()) {
		if (this->tx_message_vec->size() != 0) {
			auto itr = this->tx_message_vec->begin();
			Message* message = *itr;
			this->inject_message(message);
			this->transmitted_messages_vec->push_back(message->message_id);
			global_message_transmission_info[message->message_id]->tx_time = (int)global_clock;
			this->tx_message_vec->erase(itr);
		}
	}

	// if injection buffer is not empty, propose transmission on router output channel
	if (!this->injection_buffer->is_empty() && this->router_output_channel->is_open_for_transmission()) {
		this->transmit_message_flag = true;
		this->router_output_channel->propose_transmission(this->injection_buffer);
	}
}

void Processor::rx () {
	// look at router input channel and if it is closed for transmission, then execute transmission
	if (!(this->router_input_channel->is_open_for_transmission())) {
		this->receive_message_flag = true;
		FLIT_TYPE flit_type = this->router_input_channel->execute_transmission(this->router_buffer);
		if (flit_type == TAIL) this->router_input_channel->reset_transmission_state();

		// remove flit from buffer and adjust message id to num flits map
		Flit* flit = this->router_buffer->remove_flit();
		this->num_flits_received++;

		// if flit was a head flit, increment avg distance
		if (flit->type == HEAD) {
			Head_Flit* head_flit = (Head_Flit*)flit;
			float normalized_distance = (float)head_flit->distance / (float)head_flit->num_packets;
			global_message_transmission_info[flit->message_id]->avg_packet_distance += normalized_distance;
		}
		auto itr = this->rx_message_id_to_num_flits_map->find(flit->message_id);
		assert(itr != this->rx_message_id_to_num_flits_map->end());
		itr->second--;
		// check if we received the entire message
		if (itr->second == 0) {
			this->received_messages_vec->push_back(flit->message_id);
			global_message_transmission_info[flit->message_id]->rx_time = (int)global_clock;
		}

		else if (itr->second < 0) assert(false);
	}
}

bool Processor::did_transmit_message () {
	bool transmit_message_flag = this->transmit_message_flag;
	this->transmit_message_flag = false;
	return transmit_message_flag;
}

bool Processor::did_receive_message () {
	bool receive_message_flag = this->receive_message_flag;
	this->receive_message_flag = false;
	return receive_message_flag;
}

Router::Router (uint32_t node_id, 
				void* network_id,
				uint32_t num_channels, 
				uint32_t num_neighbors, 
				uint32_t max_buffer_capacity,
				uint32_t num_virtual_channels,
				Routing_Func routing_func,
				TX_Flow_Control_Func tx_flow_control_func,
				RX_Flow_Control_Func rx_flow_control_func) :
Node(node_id, network_id, num_channels, num_neighbors, max_buffer_capacity, ROUTER) {

	this->num_virtual_channels = num_virtual_channels;
	this->routing_func = routing_func;
	this->tx_flow_control_func = tx_flow_control_func;
	this->rx_flow_control_func = rx_flow_control_func;
	this->input_channel_to_buffers_map = new std::map<Channel*, Buffer**>;
	this->neighbor_to_io_channels_map = new std::map<Router*, std::vector<IO_Channel*>*>;
	this->flit_info_to_router_id_map = new std::map<Flit_Info*, uint32_t, flit_info_comp>;
	this->internal_info_summary = new Internal_Info_Summary;
}

void Router::init_connection (Node* node, Channel* input_channel, Channel* output_channel) {
	if (node->type == ROUTER || node->type == PROCESSOR_ROUTER) this->init_router_connection((Router*)node, input_channel, output_channel);
	// should never come here
	else assert(false);
}

void Router::init_router_connection (Router* neighbor, Channel* input_channel, Channel* output_channel) {
	// add to input_channel_to_buffers_map
	Buffer** input_channel_buffers = new Buffer*[this->num_virtual_channels];
	for (uint32_t i=0; i < this->num_virtual_channels; i++) {
		Buffer* new_buffer = new Buffer(this->max_buffer_capacity);
		input_channel_buffers[i] = new_buffer;
		this->internal_info_summary->init_buffer_in_map(new_buffer);
	}
	this->input_channel_to_buffers_map->insert({input_channel, input_channel_buffers});

	// add to neighbor_to_io_channels_map
	IO_Channel* neighbor_io_channel = new IO_Channel;
	neighbor_io_channel->input_channel = input_channel;
	neighbor_io_channel->output_channel = output_channel;
	// if neighbor already in map, then add io channel to vector
	if (this->neighbor_to_io_channels_map->find(neighbor) != this->neighbor_to_io_channels_map->end()) {
		assert(this->num_virtual_channels > 1);
		auto itr = this->neighbor_to_io_channels_map->find(neighbor);
		std::vector<IO_Channel*>* io_channel_vec = itr->second;
		io_channel_vec->push_back(neighbor_io_channel);
	}
	// neighbor not in map, so need to create new vector
	else {
		std::vector<IO_Channel*>* neighbor_io_channel_vec = new std::vector<IO_Channel*>;
		neighbor_io_channel_vec->push_back(neighbor_io_channel);
		this->neighbor_to_io_channels_map->insert({neighbor, neighbor_io_channel_vec});
	}
}

std::vector<IO_Channel*>* Router::get_io_channel_vec(uint32_t neighbor_router_id) {
	// if neighbor id == node id, then this is the id of the connectect processor
	if (neighbor_router_id	== this->node_id) {
		assert(this->type == PROCESSOR_ROUTER);
		Processor_Router* processor_router = (Processor_Router*)this;
		return processor_router->processor_io_channel_vec;
	}

	for (auto itr=this->neighbor_to_io_channels_map->begin(); itr != this->neighbor_to_io_channels_map->end(); itr++) {
		Router* neighbor_router = itr->first;
		if (neighbor_router->node_id == neighbor_router_id) {
			return itr->second;
		}
	}
	// should never come here
	raise(SIGTRAP);
	assert(false);
}

void Router::update_internal_info_summary () {
	this->internal_info_summary->clear();

	for (auto itr_channel=this->input_channel_to_buffers_map->begin(); itr_channel != this->input_channel_to_buffers_map->end(); itr_channel++) {
		Buffer** buffers = itr_channel->second;
		for (uint32_t i=0; i < this->num_virtual_channels; i++) {
			Buffer* buffer = buffers[i];
			for (auto itr_buffer=buffer->begin(); itr_buffer != buffer->end(); itr_buffer++) {
				uint32_t message_id = (*itr_buffer)->message_id;
				uint32_t packet_id = (*itr_buffer)->packet_id;
				this->internal_info_summary->insert(buffer, message_id, packet_id);
			}
			this->internal_info_summary->buffer_space_occupied += buffer->occupied_size();
			this->internal_info_summary->buffer_space_total += buffer->total_size();
		}
		Channel* input_channel = itr_channel->first;
		if (input_channel->is_failed_transmission()) this->internal_info_summary->num_failed_transmissions += 1;
		input_channel->clear_transmission_status();
	}
}

uint32_t Router::get_buffer_space_occupied () {return this->internal_info_summary->buffer_space_occupied;}

uint32_t Router::get_buffer_space_total () {return this->internal_info_summary->buffer_space_total;}

uint32_t Router::get_num_failed_transmissions () {return this->internal_info_summary->num_failed_transmissions;}

void Router::clear_internal_info_summary () {
	this->internal_info_summary->clear();
}

Processor_Router::Processor_Router (uint32_t node_id, 
									void* network_id,
									uint32_t num_channels, 
									uint32_t num_neighbors, 
									uint32_t max_buffer_capacity, 
									uint32_t num_virtual_channels,
									Routing_Func routing_func,
									TX_Flow_Control_Func tx_flow_control_func,
									RX_Flow_Control_Func rx_flow_control_func) : 
Router(node_id, network_id,num_channels, num_neighbors, max_buffer_capacity, num_virtual_channels, routing_func, tx_flow_control_func, rx_flow_control_func) {
	this->type = PROCESSOR_ROUTER;
}

void Processor_Router::init_connection(Node* node, Channel* input_channel, Channel* output_channel) {
	if (node->type == PROCESSOR) this->init_processor_connection((Processor*)node, input_channel, output_channel);
	else if (node->type == ROUTER || node->type == PROCESSOR_ROUTER) this->init_router_connection((Router*)node, input_channel, output_channel);
}

void Processor_Router::init_processor_connection (Processor* processor, Channel* input_channel, Channel* output_channel) {
	this->processor = processor;

	this->processor_buffer_lst = new Buffer*[this->num_virtual_channels];
	for (uint32_t i=0; i < this->num_virtual_channels; i++) {
		Buffer* new_buffer = new Buffer(this->max_buffer_capacity);
		this->processor_buffer_lst[i] = new_buffer;
		this->internal_info_summary->init_buffer_in_map(new_buffer);
	}
	input_channel_to_buffers_map->insert({input_channel, this->processor_buffer_lst});

	IO_Channel* processor_io_channel = new IO_Channel;
	processor_io_channel->input_channel = input_channel;
	processor_io_channel->output_channel = output_channel;
	this->processor_io_channel_vec = new std::vector<IO_Channel*>;
	this->processor_io_channel_vec->push_back(processor_io_channel);
}

void Internal_Info_Summary::print () {
	for(auto itr_message=this->message_id_to_packet_id_set_map->begin(); itr_message != this->message_id_to_packet_id_set_map->end(); itr_message++) {
		printf("\tmessage %d\n", itr_message->first);
		printf("\tpackets ");
		for (auto itr_packet=itr_message->second->begin(); itr_packet != itr_message->second->end(); itr_packet++) {
			printf("%d,", *itr_packet);
		}
		printf("\n");
		printf("\t+++++++++++++++\n");
	}
	return;
}

/*
ROUTER 0
ROUTER_CONNECTIONS 1,3,4
	MESSAGE 1
	PACKETS 12,14,15
	NUM_FLITS 45
	++++++++++++++++++
	MESSAGE 3
	PACKETS 12,14,15
	NUM_FLITS 12
	++++++++++++++++++
#############################
*/
void Router::print () {
	printf("ROUTER %d\n", this->node_id);
	printf("Router Connections ");
	for (auto itr=this->neighbor_to_io_channels_map->begin(); itr != this->neighbor_to_io_channels_map->end(); itr++) {
		Router* router = itr->first;
		printf("%d,", router->node_id);
	}
	printf("\n");
	printf("##############################\n");
}

/*
PROCESSOR 0
ROUTER_CONNECTIONS 3
	NUM_FLITS_TRANSMITTED 21
	MESSAGES_TRANSMITTED 3,5,6
	+++++++++++++++++++++++
	NUM_FLITS_RECEIVED 12
	MESSAGES_RECEIVED 1,5,6
	+++++++++++++++++++++++
#############################

*/
void Processor::print () {
	printf("PROCESSOR %d\n", this->node_id);
	printf("Router Connections %d\n", this->router->node_id);
	printf("\tNum Flits Transmitted %d\n", this->num_flits_transmitted);
	printf("\tMessages Transmitted ");
	for (auto itr=this->transmitted_messages_vec->begin(); itr != this->transmitted_messages_vec->end(); itr++) {
		printf("%d,", *itr);
	}
	printf("\n");
	printf("\t+++++++++++++++\n");
	printf("\tNum Flits Received %d\n", this->num_flits_received);
	printf("\tMessages Received ");
	for (auto itr=this->received_messages_vec->begin(); itr != this->received_messages_vec->end(); itr++) {
		printf("%d,", *itr);
	}
	printf("\n");
	printf("\t+++++++++++++++\n");
}

