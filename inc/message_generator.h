#ifndef MESSAGE_GENERATOR_H
#define MESSAGE_GENERATOR_H

#include <map>
#include <vector>
#include <omp.h>

#include "message.h"

typedef enum { NODE_UNIFORM, NODE_RANDOM } MESSAGE_NODE_DISTRIBUTION;
typedef enum { SIZE_UNIFORM, SIZE_RANDOM} MESSAGE_SIZE_DISTRIBUTION;

class Message_Generator {
private:
	uint32_t num_processors;
	uint32_t lower_message_size;
	uint32_t upper_message_size;
	MESSAGE_NODE_DISTRIBUTION message_node_distribution;
	MESSAGE_SIZE_DISTRIBUTION message_size_distribution;
	std::map<uint32_t, std::vector<Message*>*>* processor_id_to_tx_message_data_map;
	std::map<uint32_t, std::map<uint32_t, int>*>* processor_id_to_rx_message_data_map;
	uint32_t* message_size_lst;
	uint32_t* num_messages_tx_by_processor;
	uint32_t* num_messages_rx_by_processor;
	omp_lock_t* tx_message_data_map_locks;
	omp_lock_t* rx_message_data_map_locks;

public:
	uint32_t num_messages;

	Message_Generator(uint32_t rng_seed,
					  uint32_t num_messages,
					  uint32_t num_processors,
					  uint32_t lower_message_size, 
					  uint32_t upper_message_size, 
					  MESSAGE_SIZE_DISTRIBUTION message_size_distribution,
					  MESSAGE_NODE_DISTRIBUTION message_node_distribution);
	void update_tx_rx_data(Message* message);
	void random_message_size_distribution_generator();
	void uniform_message_size_distribution_generator();
	void random_message_node_distribution_generator();
	void uniform_message_node_distribution_generator();
	std::vector<Message*>* get_tx_message_data_vec(uint32_t processor_id);
	std::map<uint32_t, int>* get_rx_message_data_map(uint32_t processor_id);

};

#endif /* MESSAGE_GENERATOR_H */