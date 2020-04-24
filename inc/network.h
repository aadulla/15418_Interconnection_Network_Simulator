#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <vector>
#include <map>

#include "flow_control_algorithms.h"
#include "routing_algorithms.h"
#include "node.h"

typedef enum { MESH } NETWORK_TYPE;

typedef struct _Mesh_Info {
	uint32_t num_rows;
	uint32_t num_cols;
} Mesh_Info;

class Network {

protected:
	uint32_t input_buffer_capacity;
	uint32_t router_buffer_capacity;
	uint32_t num_virtual_channels;

public: 
	uint32_t num_processors;
	uint32_t num_routers;
	Processor** processor_lst;
	Router** router_lst;
	Network(uint32_t num_processors, 
			uint32_t num_routers, 
			uint32_t input_buffer_capacity, 
			uint32_t router_buffer_capacity, 
			uint32_t num_virtual_channels);
	void init_connection(Node* node_A, Node* node_B);
	void simulate();
	virtual void print() {};

};

typedef struct _Mesh_ID {
	uint32_t x;
	uint32_t y;
} Mesh_ID;

class Mesh_Network: public Network {

private:
	Processor*** processor_mesh;
	Processor_Router*** router_mesh;
	uint32_t num_rows;
	uint32_t num_cols;

public:
	Mesh_Network(uint32_t num_processors, 
				 uint32_t num_routers, 
				 uint32_t input_buffer_capacity, 
				 uint32_t router_buffer_capacity, 
				 uint32_t num_virtual_channels,
				 Routing_Func routing_func, 
				 TX_Flow_Control_Func tx_flow_control_func, 
				 RX_Flow_Control_Func rx_flow_control_func);
	void print();

};

#endif /* NETWORK_H */