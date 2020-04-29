#include <stdint.h>
#include <math.h>
#include <vector>
#include <omp.h>
#include <signal.h>

#include "network.h"
#include "flow_control_algorithms.h"
#include "routing_algorithms.h"
#include "node.h"
#include "channel.h"

NETWORK_TYPE network_type;
void* network_info;

Network::Network (uint32_t num_processors, 
				  uint32_t num_routers, 
				  uint32_t input_buffer_capacity, 
				  uint32_t router_buffer_capacity, 
				  uint32_t num_virtual_channels) {

	this->num_processors = num_processors;
	this->num_routers = num_routers;
	this->input_buffer_capacity	= input_buffer_capacity;
	this->router_buffer_capacity = router_buffer_capacity;
	this->num_virtual_channels = num_virtual_channels;
	this->processor_lst = new Processor*[this->num_processors];
	this->router_lst = new Router*[this->num_routers];
}

void Network::init_connection (Node* node_A, Node* node_B) {
	// node_A output channel, node_B input channel
	Channel* channel_A_B = new Channel(node_A, node_B);
	// node_A input channel, node_B output channel
	Channel* channel_B_A = new Channel(node_B, node_A);
	// node_A connection
	node_A->init_connection(node_B, channel_B_A, channel_A_B);
	// node_B connection
	node_B->init_connection(node_A, channel_A_B, channel_B_A);
}

// parallelize this
void Network::simulate () {
	#pragma omp parallel for schedule(static)
	for (uint32_t i=0; i < this->num_routers; i++) {
		Router* router = this->router_lst[i];
		router->clear_internal_info_summary();
	}
	#pragma omp parallel for schedule(static)
	for (uint32_t i=0; i < this->num_processors; i++) {
		Processor* processor = this->processor_lst[i];
		processor->tx();
	}
	#pragma omp parallel for schedule(static)
	for (uint32_t i=0; i < this->num_routers; i++) {
		Router* router = this->router_lst[i];
		router->tx();
	}
	#pragma omp parallel for schedule(static)
	for (uint32_t i=0; i < this->num_routers; i++) {
		Router* router = this->router_lst[i];
		router->rx();
	}
	#pragma omp parallel for schedule(static)
	for (uint32_t i=0; i < this->num_processors; i++) {
		Processor* processor = this->processor_lst[i];
		processor->rx();
	}
	#pragma omp parallel for schedule(static)
	for (uint32_t i=0; i < this->num_routers; i++) {
		Router* router = this->router_lst[i];
		router->update_internal_info_summary();
	}
}



Mesh_Network::Mesh_Network (uint32_t num_processors, 
							uint32_t num_routers, 
							uint32_t input_buffer_capacity, 
							uint32_t router_buffer_capacity, 
							uint32_t num_virtual_channels,
							Routing_Func routing_func, 
							Flow_Control_Func flow_control_func, 
							FLOW_CONTROL_GRANULARITY flow_control_granularity) : 
Network(num_processors, 
		num_routers, 
		input_buffer_capacity, 
		router_buffer_capacity, 
		num_virtual_channels) {

	this->num_rows = sqrt(this->num_processors);
	this->num_cols = sqrt(this->num_processors);

	// set global vars to identify mesh Network
	network_type = MESH;
	Mesh_Info* mesh_info = new Mesh_Info;
	mesh_info->num_rows = this->num_rows;
	mesh_info->num_cols = this->num_cols;
	network_info = (void*)mesh_info;

	// create NxN array of processors, N = sqrt(num_processors)
	this->processor_mesh = new Processor**[num_rows];
	for (uint32_t i=0; i < this->num_rows; i++) {
		this->processor_mesh[i] = new Processor*[num_cols];
		for (uint32_t j=0; j < this->num_cols; j++) {
			Mesh_ID* mesh_id = new Mesh_ID;
			mesh_id->x = j;
			mesh_id->y = i;
			Processor* new_processor = new Processor(i*num_cols + j, (void*)mesh_id, 1, 1, this->input_buffer_capacity);
			this->processor_mesh[i][j] = new_processor;
			this->processor_lst[i*num_cols + j] = new_processor;
		}
	}

	// create NxN array of routers, N = sqrt(num_routers)
	this->router_mesh = new Processor_Router**[num_rows];
	for (uint32_t i=0; i < this->num_rows; i++) {
		this->router_mesh[i] = new Processor_Router*[num_cols];
		for (uint32_t j=0; j < this->num_cols; j++) {
			Mesh_ID* mesh_id = new Mesh_ID;
			mesh_id->x = j;
			mesh_id->y = i;
			Processor_Router* new_processor_router = new Processor_Router(i*num_cols + j, 
																		  (void*)mesh_id, 
																		  1, 
																		  1, 
																		  this->router_buffer_capacity, 
																		  this->num_virtual_channels,
																		  routing_func,
																		  flow_control_func,
																		  flow_control_granularity);
			this->router_mesh[i][j] = new_processor_router;
			this->router_lst[i*num_cols + j] = (Router*)new_processor_router;
		}
	}

	// connect processor and router together
	for (uint32_t i=0; i < this->num_rows; i++) {
		for (uint32_t j=0; j < this->num_cols; j++) {
			this->init_connection(this->processor_mesh[i][j], this->router_mesh[i][j]);
		}
	}

	// connect routers together
	for (uint32_t i=0; i < this->num_rows; i++) {
		for (uint32_t j=0; j < this->num_cols; j++) {
			Processor_Router* curr_router = this->router_mesh[i][j];
			if (j < num_cols-1) {
				// connect east
				this->init_connection(curr_router, this->router_mesh[i][j+1]);
			}
			if (i < num_rows - 1) {
				// conect south
				this->init_connection(curr_router, this->router_mesh[i+1][j]);
			}
		}
	}
}

void Mesh_Network::print() {
	printf("NUM ROUTERS %d\n", this->num_routers);
	printf("NUM PROCESSORS %d\n", this->num_processors);

	printf("==================================================\n");
	printf("==================================================\n");

	for (uint32_t i=0; i < this->num_rows; i++) {
		for (uint32_t j=0; j < this->num_cols; j++) {
			this->router_mesh[i][j]->print();		
		}
	}

	printf("==================================================\n");
	printf("==================================================\n");

	for (uint32_t i=0; i < this->num_rows; i++) {
		for (uint32_t j=0; j < this->num_cols; j++) {
			this->processor_mesh[i][j]->print();		
		}
	}

	printf("==================================================\n");
	printf("==================================================\n");
	printf("\n");
}