#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <map>
#include <vector>
#include <string>
#include <set>

#include "flow_control_algorithms.h"
#include "routing_algorithms.h"
#include "channel.h"
#include "message.h"

typedef enum { PROCESSOR, ROUTER, PROCESSOR_ROUTER } NODE_TYPE;

typedef struct _IO_Channel {
	Channel* input_channel;
	Channel* output_channel;
} IO_Channel;

class Internal_Info_Summary {

public:
	std::map<uint32_t, std::set<uint32_t>*>* message_id_to_packet_id_set_map;
	std::map<Buffer*, std::set<Flit_Info*, flit_info_comp>*>* buffer_to_flit_info_set_map;
	uint32_t buffer_space_occupied;
	uint32_t buffer_space_total;
	uint32_t num_failed_transmissions;

	Internal_Info_Summary();
	void init_buffer_in_map(Buffer* buffer);
	void insert(Buffer* buffer, uint32_t message_id, uint32_t packet_id);
	void clear();
	void print();

};

class Node {

protected:
	uint32_t num_channels;
	uint32_t num_neighbors;
	uint32_t max_buffer_capacity;

public:
	uint32_t node_id;
	void* network_id;
	NODE_TYPE type;
	Internal_Info_Summary* internal_info_summary;

	Node(uint32_t node_id, void* network_id, uint32_t num_channels, uint32_t num_neighbors, uint32_t max_buffer_capacity, NODE_TYPE type);
	virtual void init_connection(Node* node, Channel* input_channel, Channel* output_channel) {};

};

class Router : public Node {

protected:
	void init_router_connection(Router* router, Channel* router_input_channel, Channel* router_output_channel);

public:
	uint32_t num_virtual_channels;
	Routing_Func routing_func;
	TX_Flow_Control_Func tx_flow_control_func;
	RX_Flow_Control_Func rx_flow_control_func;
	std::map<Channel*, Buffer**>* input_channel_to_buffers_map;
	std::map<Router*, std::vector<IO_Channel*>*>* neighbor_to_io_channels_map;
	std::map<Flit_Info*, uint32_t, flit_info_comp>* flit_info_to_router_id_map;
	Internal_Info_Summary* internal_info_summary;

	Router(uint32_t node_id, 
		   void* network_id,
		   uint32_t num_channels, 
		   uint32_t num_neighbors, 
		   uint32_t max_buffer_capacity, 
		   uint32_t num_virtual_channels, 
		   Routing_Func routing_func,
		   TX_Flow_Control_Func tx_flow_control_func,
		   RX_Flow_Control_Func rx_flow_control_func);
	void init_connection(Node* node, Channel* input_channel, Channel* output_channel);
	std::vector<IO_Channel*>* get_io_channel_vec(uint32_t neighbor_router_id);
	void update_internal_info_summary();
	uint32_t get_buffer_space_occupied();
	uint32_t get_buffer_space_total();
	uint32_t get_num_failed_transmissions();
	void clear_internal_info_summary();
	void print();

};

class Processor : public Node {

private:
	Router* router;
	Channel* router_input_channel;
	Channel* router_output_channel;
	Buffer* injection_buffer;
	Buffer* router_buffer;
	bool transmit_message_flag;
	bool receive_message_flag;

	void init_router_connection(Router* router, Channel* input_channel, Channel* output_channel);

public:
	std::vector<Message*>* tx_message_vec;
	std::map<uint32_t, int>* rx_message_id_to_num_flits_map;
	uint32_t num_flits_transmitted;
	std::vector<uint32_t>* transmitted_messages_vec;
	std::vector<uint32_t>* received_messages_vec;
	uint32_t num_flits_received;

	Processor(uint32_t node_id, 
			  void* network_id,
			  uint32_t num_channels, 
			  uint32_t num_neighbors, 
			  uint32_t max_buffer_capacity);
	void init_tx_message_vec(std::vector<Message*>* tx_message_vec);
	void init_rx_message_map(std::map<uint32_t, int>* rx_message_id_to_num_flits_map);
	void init_connection(Node* node, Channel* input_channel, Channel* output_channel);
	void inject_message(Message* message);
	void tx();
	void rx();
	bool did_transmit_message();
	bool did_receive_message();
	void print();

	// void dummy_update();
};

class Processor_Router : public Router {

public:
	Processor* processor;
	Buffer** processor_buffer_lst;
	std::vector<IO_Channel*>* processor_io_channel_vec;

	Processor_Router(uint32_t node_id, 
					 void* network_id,
					 uint32_t num_channels, 
					 uint32_t num_neighbors, 
					 uint32_t max_buffer_capacity, 
					 uint32_t num_virtual_channels, 
					 Routing_Func routing_func,
					 TX_Flow_Control_Func tx_flow_control_func,
					 RX_Flow_Control_Func rx_flow_control_func);
	void init_connection(Node* node, Channel* input_channel, Channel* output_channel);
	void init_processor_connection(Processor* processor, Channel* input_channel, Channel* output_channel);
};


#endif /* NODE_H */