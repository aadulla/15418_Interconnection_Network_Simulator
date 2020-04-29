#include <string>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <omp.h>
#include <fstream>
#include <iostream>

#include "simulator.h"
#include "network.h"
#include "message_generator.h"
#include "config_parser.h"

uint32_t packet_width;
uint32_t num_data_flits_per_packet;

uint32_t global_clock;
Message_Transmission_Info** global_message_transmission_info;

Simulator::Simulator(std::string test_path) {
	this->test_path = test_path;
	this->config_file_path = test_path + "config.txt";
	this->tx_stats_path = test_path + "tx_stats.txt";
	this->rx_stats_path = test_path + "rx_stats.txt";
	this->stalls_stats_path = test_path + "stalls_stats.txt";
	this->buffers_stats_path = test_path + "buffers_stats.txt";
	this->transmissions_stats_path = test_path + "transmissions_stats.txt";
	this->aggregate_stats_path = test_path + "aggregate_stats.txt";

	this->network = NULL;
	this->message_generator = NULL;
	this->config_parser = NULL;
	this->is_simulation_finished = false;
	this->num_threads = omp_get_num_threads();

	this->total_message_latency = 0;
	this->total_message_distance = 0.0;
	this->total_message_size = 0;
	this->avg_message_latency = 0.0;
	this->avg_message_distance = 0.0;
	this->avg_message_size = 0;
	this->avg_message_throughput = 0.0;
	this->avg_message_speed = 0.0;
	this->tx_flits_over_time_vec = new std::vector<uint32_t>;
	this->rx_flits_over_time_vec = new std::vector<uint32_t>;
	this->stalls_over_time_vec = new std::vector<uint32_t>;
	this->buffers_efficiency_over_time_vec = new std::vector<float>;
}

void Simulator::setup() {
	std::cout << "Starting Simulation Setup...\n" << std::endl;
	std::cout << "Reading Config From Test Directory: " << this->config_file_path << std::endl;
	std::cout << std::endl;

	// initialize global clock
	global_clock = 0;
	
	// initialize config parser
	this->config_parser = new Config_Parser();
	this->config_parser->initialize_parameter_key("Network Type");
	this->config_parser->initialize_parameter_key("Number of Processors");
	this->config_parser->initialize_parameter_key("Number of Routers");
	this->config_parser->initialize_parameter_key("Router Buffer Capacity");
	this->config_parser->initialize_parameter_key("Number of Virtual Channels");
	this->config_parser->initialize_parameter_key("Packet Width");
	this->config_parser->initialize_parameter_key("Number of Data Flits Per Packet");
	this->config_parser->initialize_parameter_key("Routing Algorithm");
	this->config_parser->initialize_parameter_key("Flow Control Algorithm");
	this->config_parser->initialize_parameter_key("Flow Control Granularity");
	this->config_parser->initialize_parameter_key("Number of Messages");
	this->config_parser->initialize_parameter_key("Lower Message Size");
	this->config_parser->initialize_parameter_key("Upper Message Size");
	this->config_parser->initialize_parameter_key("Message Size Distribution");
	this->config_parser->initialize_parameter_key("Message Node Distribution");

	// read config file
	this->config_parser->parse_config_file(this->config_file_path);
	std::cout << "Configuration Parameters: " << std::endl;
	this->config_parser->print();
	std::cout << std::endl;

	// initialize global vars
	packet_width = this->config_parser->get_int_parameter_value("Packet Width");
	num_data_flits_per_packet = this->config_parser->get_int_parameter_value("Number of Data Flits Per Packet");

	// get message generator parameters
	uint32_t num_messages = this->config_parser->get_int_parameter_value("Number of Messages");
	uint32_t num_processors = this->config_parser->get_int_parameter_value("Number of Processors");
	uint32_t lower_message_size = this->config_parser->get_int_parameter_value("Lower Message Size");
	uint32_t upper_message_size = this->config_parser->get_int_parameter_value("Upper Message Size");
	std::string message_size_distribution_str = this->config_parser->get_string_parameter_value("Message Size Distribution");
	std::string message_node_distribution_str = this->config_parser->get_string_parameter_value("Message Node Distribution");

	// initialize message size distribution
	MESSAGE_SIZE_DISTRIBUTION message_size_distribution;
	if (message_size_distribution_str.compare("Random") == 0) {
		message_size_distribution = SIZE_RANDOM;
	}
	else if (message_size_distribution_str.compare("Uniform") == 0) {
		message_size_distribution = SIZE_UNIFORM;
	}
	// should never come here
	else {
		raise(SIGTRAP);
		assert(false);
	}

	// initialize message node distribution
	MESSAGE_NODE_DISTRIBUTION message_node_distribution;
	if (message_node_distribution_str.compare("Random") == 0) {
		message_node_distribution = NODE_RANDOM;
	}
	else if (message_node_distribution_str.compare("Uniform") == 0) {
		message_node_distribution = NODE_UNIFORM;
	}
	// should never come here
	else assert(false);

	// initialize global message transmission info
	global_message_transmission_info = new Message_Transmission_Info*[num_messages];
	for (uint32_t i=0; i < num_messages; i++) {
		Message_Transmission_Info* message_transmission_info = new Message_Transmission_Info;
		message_transmission_info->tx_processor_id = 0;
		message_transmission_info->tx_time = -1;
		message_transmission_info->rx_processor_id = 0;
		message_transmission_info->rx_time = -1;
		global_message_transmission_info[i] = message_transmission_info;
	}

	// initialize message generator
	this->message_generator	= new Message_Generator(num_messages,
					 				  				num_processors,
					  				  				lower_message_size, 
					  				  				upper_message_size, 
					  				  				message_size_distribution,
					  				  				message_node_distribution);

	// get network parameters
	std::string network_type = this->config_parser->get_string_parameter_value("Network Type");
	uint32_t num_routers = this->config_parser->get_int_parameter_value("Number of Routers");
	std::string routing_algo_str = this->config_parser->get_string_parameter_value("Routing Algorithm");
	std::string flow_control_algo_str = this->config_parser->get_string_parameter_value("Flow Control Algorithm");
	std::string flow_control_granularity_str = this->config_parser->get_string_parameter_value("Flow Control Granularity");
	uint32_t input_buffer_capacity = (uint32_t)((upper_message_size / packet_width) * (num_data_flits_per_packet + 2));
	uint32_t router_buffer_capacity = this->config_parser->get_int_parameter_value("Router Buffer Capacity");
	uint32_t num_virtual_channels = this->config_parser->get_int_parameter_value("Number of Virtual Channels");

	// initialize routing functions
	Routing_Func routing_func;
	if (routing_algo_str.compare("Mesh XY") == 0) {
		routing_func = &mesh_xy_routing;
	}
	else if (routing_algo_str.compare("Mesh YX") == 0) {
		routing_func = &mesh_yx_routing;
	}
	else if (routing_algo_str.compare("Mesh Adaptive") == 0) {
		routing_func = &mesh_adaptive_routing;
	}
	// should never come here
	else assert(false);

	// initilize flow control function
	Flow_Control_Func flow_control_func;
	if (flow_control_algo_str.compare("Cut Through") == 0) {
		flow_control_func = &cut_through_flow_control;
	}
	else if (flow_control_algo_str.compare("Store Forward") == 0) {
		flow_control_func = &store_forward_flow_control;
	}
	// should never come here
	else assert(false);

	// initilize flow control granularity
	FLOW_CONTROL_GRANULARITY flow_control_granularity;
	if (flow_control_granularity_str.compare("Packet") == 0) {
		flow_control_granularity = PACKET;
	}
	else if (flow_control_granularity_str.compare("Flit") == 0) {
		flow_control_granularity = FLIT;
	}
	// should never come here
	else assert(false);



	// initialize network
	if (network_type.compare("Mesh") == 0) {
		this->network = new Mesh_Network(num_processors, 
										 num_routers, 
										 input_buffer_capacity,
										 router_buffer_capacity,
										 num_virtual_channels,
										 routing_func, 
										 flow_control_func, 
										 flow_control_granularity);
	}
	// should never come here
	else assert(false);

	// transfer messages into processor data structures
	for (uint32_t i=0; i < num_processors; i++) {
		this->network->processor_lst[i]->init_tx_message_vec(this->message_generator->get_tx_message_data_vec(i));
		this->network->processor_lst[i]->init_rx_message_map(this->message_generator->get_rx_message_data_map(i));
	}

	printf("Finished Simulation Setup!!!\n\n");

}

void Simulator::update_simulation_status () {
	for (uint32_t i=0; i < this->message_generator->num_messages; i++) {
		int rx_time = global_message_transmission_info[i]->rx_time;
		if (rx_time < 0) return;
	}
	this->is_simulation_finished = true;
}

void Simulator::update_over_time_metrics () {
	uint32_t sum_tx_flits = 0;
	uint32_t sum_rx_flits = 0;
	uint32_t sum_num_stalls = 0;
	uint32_t sum_buffers_space_occupied = 0;
	uint32_t sum_buffers_space_total = 0;

	#pragma omp parallel 
	{
		uint32_t thread_tx_flits = 0;
		uint32_t thread_rx_flits = 0;
		uint32_t thread_num_stalls = 0;
		uint32_t thread_buffers_space_occupied = 0;
		uint32_t thread_buffers_space_total = 0;
		
		#pragma omp for schedule(static) nowait
		for (uint32_t i=0; i < this->network->num_processors; i++) {
			Processor* processor = this->network->processor_lst[i];
			if (processor->did_transmit_message()) thread_tx_flits += 1;
			if (processor->did_receive_message()) thread_rx_flits += 1;
		}

		#pragma omp for schedule(static) nowait
		for (uint32_t i=0; i < this->network->num_routers; i++) {
			Router* router = this->network->router_lst[i];
			thread_num_stalls += router->get_num_stalls();
			thread_buffers_space_occupied += router->get_buffer_space_occupied();
			thread_buffers_space_total += router->get_buffer_space_total();
		}

		#pragma omp atomic
		sum_tx_flits += thread_tx_flits;
		#pragma omp atomic
		sum_rx_flits += thread_rx_flits;
		#pragma omp atomic
		sum_num_stalls += thread_num_stalls;
		#pragma omp atomic
		sum_buffers_space_occupied += thread_buffers_space_occupied;
		#pragma omp atomic
		sum_buffers_space_total += thread_buffers_space_total;
	}

	// printf("Num Transmitted %d\n", sum_tx_flits);
	// printf("Num Received %d\n", sum_rx_flits);
	// printf("Num Flits in Network %d\n", sum_buffers_space_occupied);
	// printf("\n");

	this->tx_flits_over_time_vec->push_back(sum_tx_flits);
	this->rx_flits_over_time_vec->push_back(sum_rx_flits);
	this->stalls_over_time_vec->push_back(sum_num_stalls);
	float buffers_efficiency = (float)sum_buffers_space_occupied / (float)sum_buffers_space_total;
	this->buffers_efficiency_over_time_vec->push_back(buffers_efficiency);
}

void Simulator::update_aggregate_metrics () {
	uint32_t num_messages = this->message_generator->num_messages;

	#pragma omp parallel 
	{
		uint32_t thread_message_latency = 0;
		float thread_message_distance = 0;
		uint32_t thread_message_size = 0;

		#pragma omp for schedule(static) nowait
		for (uint32_t i=0; i < num_messages; i++) {
			Message_Transmission_Info* message_transmission_info = global_message_transmission_info[i];
			thread_message_latency += message_transmission_info->latency;
			thread_message_distance += message_transmission_info->avg_packet_distance;
			thread_message_size += message_transmission_info->size;
		}

		#pragma omp atomic
		this->total_message_latency += thread_message_latency;
		#pragma omp atomic
		this->total_message_distance += thread_message_distance;
		#pragma omp atomic
		this->total_message_size += thread_message_size;
	}

	this->avg_message_latency = (float)this->total_message_latency / (float)num_messages;
	this->avg_message_distance = this->total_message_distance / (float)num_messages;
	this->avg_message_size = this->total_message_size / (float)num_messages;
	this->avg_message_throughput = (float)num_messages / (float)global_clock;
	this->avg_message_speed = this->avg_message_distance / this->avg_message_latency;
}

void Simulator::simulate () {
	printf("Staring Simulation...\n\n");

	while (!this->is_simulation_finished) {
		// printf("Starting Global Clock Cycle %d\n", global_clock);
		this->network->simulate();
		// this->network->print();
		// this->print_global_message_transmission_info();
		// printf("Finished Global Clock Cycle %d\n\n", global_clock);
		global_clock++;
		this->update_over_time_metrics();
		this->update_simulation_status();
		// raise(SIGTRAP);
	}
	printf("Finished Simulation!!!\n\n");
	this->print_global_message_transmission_info();

	this->update_aggregate_metrics();
	this->print_aggregate_metrics();
	printf("Total Simulation Time in Clock Cycles: %d\n\n", global_clock);

	printf("Starting Logging Stats...\n\n");
	this->log_stats();
	printf("Finished Logging Stats!!!\n\n");

}

void Simulator::log_stats () {
	std::ofstream tx_stats_file(this->tx_stats_path);
	for (auto itr=this->tx_flits_over_time_vec->begin(); itr != this->tx_flits_over_time_vec->end(); itr++) {
		uint32_t val = *itr;
		tx_stats_file << val << std::endl;
	}
	tx_stats_file.close();

	std::ofstream rx_stats_file(this->rx_stats_path);
	for (auto itr=this->rx_flits_over_time_vec->begin(); itr != this->rx_flits_over_time_vec->end(); itr++) {
		uint32_t val = *itr;
		rx_stats_file << val << std::endl;
	}
	rx_stats_file.close();

	std::ofstream stalls_stats_file(this->stalls_stats_path);
	for (auto itr=this->stalls_over_time_vec->begin(); itr != this->stalls_over_time_vec->end(); itr++) {
		uint32_t val = *itr;
		stalls_stats_file << val << std::endl;
	}
	stalls_stats_file.close();

	std::ofstream buffers_stats_file(this->buffers_stats_path);
	for (auto itr=this->buffers_efficiency_over_time_vec->begin(); itr != this->buffers_efficiency_over_time_vec->end(); itr++) {
		float val = *itr;
		buffers_stats_file << val << std::endl;
	}
	buffers_stats_file.close();

	std::ofstream transmissions_stats_file(this->transmissions_stats_path);
	transmissions_stats_file << "Latency" << " " \
							 << "Size" << " " \
							 << "Distance" << " " \
							 << "TX_Processor_ID" << " " \
							 << "TX_Time" << " " \
							 << "RX_Processor_ID" << " " \
							 << "RX_Time" << std::endl;
	for (uint32_t i=0; i < this->message_generator->num_messages; i++) {
		Message_Transmission_Info* message_transmission_info = global_message_transmission_info[i];

		uint32_t latency = message_transmission_info->latency;
		uint32_t size = message_transmission_info->size;
		uint32_t distance = (uint32_t)message_transmission_info->avg_packet_distance;
		uint32_t tx_processor_id = message_transmission_info->tx_processor_id;
		uint32_t tx_time = message_transmission_info->tx_time;
		uint32_t rx_processor_id = message_transmission_info->rx_processor_id;
		uint32_t rx_time = message_transmission_info->rx_time;

		transmissions_stats_file << latency << " " \
								 << size << " " \
								 << distance << " " \
								 << tx_processor_id << " " \
								 << tx_time << " " \
								 << rx_processor_id << " " \
								 << rx_time << std::endl;
	}
	transmissions_stats_file.close();

	std::ofstream aggregate_stats_file(this->aggregate_stats_path);
	aggregate_stats_file << "Average_Message_Latency_In_Clock_Cycles" << " " \
						 << "Average_Message_Distance_In_Channels" << " " \
						 << "Average_Message_Size" << " " \
						 << "Average_Message_Throughput_In_Messages/Clock_Cycles" << " " \
						 << "Average_Message_Speed_In_Distance/Latency" << std::endl;
	aggregate_stats_file << this->avg_message_latency << " " \
						 << this->avg_message_distance << " " \
						 << this->avg_message_size << " " \
						 << this->avg_message_throughput << " " \
						 << this->avg_message_speed << std::endl;
	aggregate_stats_file.close();
}

void Simulator::print_global_message_transmission_info () {
	printf("Global Clock Cycle %d\n", global_clock);
	printf("\n");
	printf("%-15s%-12s%-10s%-25s%-20s%-12s%-20s%-12s\n", "Message ID", "Latency", "Size", "Avg Packet Distance", "TX Processor ID", "TX Time", "RX Processor ID", "RX Time");
	for (uint32_t i=0; i < this->message_generator->num_messages; i++) {
		uint32_t latency = global_message_transmission_info[i]->latency;
		uint32_t size = global_message_transmission_info[i]->size;
		float avg_packet_distance = global_message_transmission_info[i]->avg_packet_distance;
		uint32_t tx_processor_id = global_message_transmission_info[i]->tx_processor_id;
		int tx_time = global_message_transmission_info[i]->tx_time;
		uint32_t rx_processor_id = global_message_transmission_info[i]->rx_processor_id;
		int rx_time = global_message_transmission_info[i]->rx_time;
		printf("%-15d%-12d%-10d%-25d%-20d%-12d%-20d%-12d\n", i, latency, size, (uint32_t)avg_packet_distance, tx_processor_id, tx_time, rx_processor_id, rx_time);
	}
	printf("\n\n");
}

void Simulator::print_aggregate_metrics() {
	printf("Average Message Latency in Clock Cycles: %f\n", this->avg_message_latency);
	printf("Average Message Distance in Channels: %f\n", this->avg_message_distance);
	printf("Average Message Size: %f\n", this->avg_message_size);
	printf("Average Throughput in Messages/Clock Cycles: %f\n", this->avg_message_throughput);
	printf("Average Speed in Distance/Latency: %f\n", this->avg_message_speed);
}
