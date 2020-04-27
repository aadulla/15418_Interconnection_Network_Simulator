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
	this->failed_stats_path = test_path + "failed_stats.txt";
	this->buffer_stats_path = test_path + "buffer_stats.txt";

	this->network = NULL;
	this->message_generator = NULL;
	this->config_parser = NULL;
	this->is_simulation_finished = false;
	this->num_threads = omp_get_num_threads();

	this->total_message_latency = 0;
	this->total_message_distance = 0.0;
	this->avg_message_latency = 0.0;
	this->avg_message_distance = 0.0;
	this->avg_message_throughput = 0.0;
	this->tx_messages_over_time_vec = new std::vector<uint32_t>;
	this->rx_messages_over_time_vec = new std::vector<uint32_t>;
	this->failed_transmissions_over_time_vec = new std::vector<uint32_t>;
	this->buffer_efficiency_over_time_vec = new std::vector<float>;
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
	uint32_t rng_seed = 15418;
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
	this->message_generator	= new Message_Generator(rng_seed,
									  				num_messages,
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
	uint32_t sum_tx_messages = 0;
	uint32_t sum_rx_messages = 0;
	uint32_t sum_num_failed_transmissions = 0;
	uint32_t sum_buffer_space_occupied = 0;
	uint32_t sum_buffer_space_total = 0;

	#pragma omp parallel 
	{
		uint32_t thread_tx_messages = 0;
		uint32_t thread_rx_messages = 0;
		uint32_t thread_num_failed_transmissions = 0;
		uint32_t thread_buffer_space_occupied = 0;
		uint32_t thread_buffer_space_total = 0;
		
		#pragma omp for schedule(static) nowait
		for (uint32_t i=0; i < this->network->num_processors; i++) {
			Processor* processor = this->network->processor_lst[i];
			if (processor->did_transmit_message()) thread_tx_messages += 1;
			if (processor->did_receive_message()) thread_rx_messages += 1;
		}

		#pragma omp for schedule(static) nowait
		for (uint32_t i=0; i < this->network->num_routers; i++) {
			Router* router = this->network->router_lst[i];
			thread_num_failed_transmissions += router->get_num_failed_transmissions();
			thread_buffer_space_occupied += router->get_buffer_space_occupied();
			thread_buffer_space_total += router->get_buffer_space_total();
		}

		#pragma omp atomic
		sum_tx_messages += thread_tx_messages;
		#pragma omp atomic
		sum_rx_messages += thread_rx_messages;
		#pragma omp atomic
		sum_num_failed_transmissions += thread_num_failed_transmissions;
		#pragma omp atomic
		sum_buffer_space_occupied += thread_buffer_space_occupied;
		#pragma omp atomic
		sum_buffer_space_total += thread_buffer_space_total;
	}

	// printf("Num Transmitted %d\n", sum_tx_messages);
	// printf("Num Received %d\n", sum_rx_messages);
	printf("Num Flits in Network %d\n", sum_buffer_space_occupied);
	// printf("\n");

	this->tx_messages_over_time_vec->push_back(sum_tx_messages);
	this->rx_messages_over_time_vec->push_back(sum_rx_messages);
	this->failed_transmissions_over_time_vec->push_back(sum_num_failed_transmissions);
	float buffer_efficiency = (float)sum_buffer_space_occupied / (float)sum_buffer_space_total;
	this->buffer_efficiency_over_time_vec->push_back(buffer_efficiency);
}

void Simulator::update_aggregate_metrics () {
	uint32_t num_messages = this->message_generator->num_messages;

	#pragma omp parallel 
	{
		uint32_t thread_message_latency = 0;
		float thread_message_distance = 0;

		#pragma omp for schedule(static) nowait
		for (uint32_t i=0; i < num_messages; i++) {
			Message_Transmission_Info* message_transmission_info = global_message_transmission_info[i];
			uint32_t message_latency = (uint32_t)(message_transmission_info->rx_time - message_transmission_info->tx_time);
			thread_message_latency += message_latency;
			thread_message_distance += message_transmission_info->avg_packet_distance;
		}

		#pragma omp atomic
		this->total_message_latency += thread_message_latency;
		#pragma omp atomic
		this->total_message_distance += thread_message_distance;
	}

	this->avg_message_latency = (float)this->total_message_latency / (float)num_messages;
	this->avg_message_distance = this->total_message_distance / (float)num_messages;
	this->avg_message_throughput = (float)num_messages / (float)global_clock;
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

	printf("Starting Logging Stats...\n\n");
	this->log_stats();
	printf("Finished Logging Stats!!!\n\n");

	this->update_aggregate_metrics();
	this->print_aggregate_metrics();
	printf("Total Simulation Time in Clock Cycles: %d\n", global_clock);


}

void Simulator::log_stats () {
	std::ofstream tx_stats_file(this->tx_stats_path);
	for (auto itr=this->tx_messages_over_time_vec->begin(); itr != this->tx_messages_over_time_vec->end(); itr++) {
		uint32_t val = *itr;
		tx_stats_file << val << std::endl;
	}
	tx_stats_file.close();

	std::ofstream rx_stats_file(this->rx_stats_path);
	for (auto itr=this->rx_messages_over_time_vec->begin(); itr != this->rx_messages_over_time_vec->end(); itr++) {
		uint32_t val = *itr;
		rx_stats_file << val << std::endl;
	}
	rx_stats_file.close();

	std::ofstream failed_stats_file(this->failed_stats_path);
	for (auto itr=this->failed_transmissions_over_time_vec->begin(); itr != this->failed_transmissions_over_time_vec->end(); itr++) {
		uint32_t val = *itr;
		failed_stats_file << val << std::endl;
	}
	failed_stats_file.close();

	std::ofstream buffer_stats_file(this->buffer_stats_path);
	for (auto itr=this->buffer_efficiency_over_time_vec->begin(); itr != this->buffer_efficiency_over_time_vec->end(); itr++) {
		float val = *itr;
		buffer_stats_file << val << std::endl;
	}
	buffer_stats_file.close();
}

void Simulator::print_global_message_transmission_info () {
	printf("Global Clock Cycle %d\n", global_clock);
	printf("\n");
	printf("%-15s%-25s%-20s%-12s%-20s%-12s\n", "Message ID", "Avg Packet Distance", "TX Processor ID", "TX Time", "RX Processor ID", "RX Time");
	for (uint32_t i=0; i < this->message_generator->num_messages; i++) {
		float avg_packet_distance = global_message_transmission_info[i]->avg_packet_distance;
		uint32_t tx_processor_id = global_message_transmission_info[i]->tx_processor_id;
		int tx_time = global_message_transmission_info[i]->tx_time;
		uint32_t rx_processor_id = global_message_transmission_info[i]->rx_processor_id;
		int rx_time = global_message_transmission_info[i]->rx_time;
		printf("%-15d%-25f%-20d%-12d%-20d%-12d\n", i, avg_packet_distance, tx_processor_id, tx_time, rx_processor_id, rx_time);
	}
	printf("\n\n");
}

void Simulator::print_aggregate_metrics() {
	printf("Average Latency in Clock Cycles: %f\n", this->avg_message_latency);
	printf("Average Distance in Channels: %f\n", this->avg_message_distance);
	printf("Average Throughput in Messages/Clock Cycles: %f\n", this->avg_message_throughput);
	printf("Average Speed in Distance/Latency: %f\n", this->avg_message_distance/this->avg_message_latency);
}

// #include <unistd.h>
// #include <ios>
// #include <iostream>
// #include <fstream>
// #include <string>
// using namespace std;
// void mem_usage(double& vm_usage, double& resident_set) {
//    vm_usage = 0.0;
//    resident_set = 0.0;
//    ifstream stat_stream("/proc/self/stat",ios_base::in); //get info from proc
//    directory
//    //create some variables to get info
//    string pid, comm, state, ppid, pgrp, session, tty_nr;
//    string tpgid, flags, minflt, cminflt, majflt, cmajflt;
//    string utime, stime, cutime, cstime, priority, nice;
//    string O, itrealvalue, starttime;
//    unsigned long vsize;
//    long rss;
//    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
//    >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
//    >> utime >> stime >> cutime >> cstime >> priority >> nice
//    >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care
//    about the rest
//    stat_stream.close();
//    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // for x86-64 is configured
//    to use 2MB pages
//    vm_usage = vsize / 1024.0;
//    resident_set = rss * page_size_kb;
// }
// int main() {
//    double vm, rss;
//    mem_usage(vm, rss);
//    cout << "Virtual Memory: " << vm << "\nResident set size: " << rss << endl;
// }