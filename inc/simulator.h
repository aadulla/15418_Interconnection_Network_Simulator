#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "network.h"
#include "message_generator.h"
#include "config_parser.h"

class Simulator {

private:
	bool is_verbose; 

	std::string test_path;
	std::string config_file_path;
	std::string tx_stats_path;
	std::string rx_stats_path;
	std::string stalls_stats_path;
	std::string buffers_stats_path;
	std::string transmissions_stats_path;
	std::string aggregate_stats_path;
	
	Network* network;
	Message_Generator* message_generator;
	Config_Parser* config_parser;
	bool is_simulation_finished;
	int num_threads;

	/* aggregate simulation metrics */
	uint32_t total_message_latency;
	float total_message_distance; // same as avg packet distance
	uint32_t total_message_size;
	float avg_message_latency;
	float avg_message_distance;
	float avg_message_size;
	float avg_message_throughput;
	float avg_message_speed;

	/* over time simulation metrics */
	std::vector<uint32_t>* tx_flits_over_time_vec; // how many flits were transmitted by processors
	std::vector<uint32_t>* rx_flits_over_time_vec; // how many flits were received by processors
	std::vector<uint32_t>* stalls_over_time_vec; // metric for contention in network
	std::vector<float>* buffers_efficiency_over_time_vec; // metric for how "busy" network is

	/* deadlock check */
	int num_flits_in_network;
	uint32_t sample_rate;

public:
	Simulator(std::string test_path, bool is_verbose);
	void setup();
	void update_over_time_metrics();
	void update_aggregate_metrics();
	void update_simulation_status();
	void simulate();
	void log_stats();
	void print_global_message_transmission_info();
	void print_aggregate_metrics();
};

#endif /* SIMULATOR_H */