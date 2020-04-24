#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "network.h"
#include "message_generator.h"
#include "config_parser.h"

class Simulator {

private:
	std::string test_path;
	std::string config_file_path;
	std::string tx_stats_path;
	std::string rx_stats_path;
	std::string failed_stats_path;
	std::string buffer_stats_path;
	
	Network* network;
	Message_Generator* message_generator;
	Config_Parser* config_parser;
	bool is_simulation_finished;
	int num_threads;

	/* aggregate simulation metrics */
	uint32_t total_message_latency;
	float total_message_distance; // same as avg packet distance
	uint32_t total_failed_transmissions;
	float avg_message_latency = 0.0;
	float avg_message_distance = 0.0;
	float avg_message_throughput = 0.0;

	/* over time simulation metrics */
	std::vector<uint32_t>* tx_messages_over_time_vec; // how many messages were transmitted by processors
	std::vector<uint32_t>* rx_messages_over_time_vec; // how many messags were received by processors
	std::vector<uint32_t>* failed_transmissions_over_time_vec; // metric for contention in network
	std::vector<float>* buffer_efficiency_over_time_vec; // metric for how "busy" network is

public:
	Simulator(std::string test_path);
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