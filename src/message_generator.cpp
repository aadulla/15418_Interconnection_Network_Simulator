#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <map>
#include <omp.h>
#include <cassert>
#include <signal.h>

#include "message_generator.h"
#include "message.h"

Message_Generator::Message_Generator (uint32_t rng_seed,
									  uint32_t num_messages,
					 				  uint32_t num_processors,
					  				  uint32_t lower_message_size, 
					  				  uint32_t upper_message_size, 
					  				  MESSAGE_SIZE_DISTRIBUTION message_size_distribution,
					  				  MESSAGE_NODE_DISTRIBUTION message_node_distribution) {

	srand(rng_seed);
	this->num_messages = num_messages;
	this->num_processors = num_processors;
	this->lower_message_size = lower_message_size;
	this->upper_message_size = upper_message_size;
	this->message_size_distribution = message_size_distribution;
	this->message_node_distribution = message_node_distribution;
	this->num_messages_tx_by_processor = new uint32_t[this->num_processors];
	this->num_messages_rx_by_processor = new uint32_t[this->num_processors];
	this->message_size_lst = new uint32_t[this->num_messages];

	// initialize locks
	this->tx_message_data_map_locks = new omp_lock_t[this->num_processors];
	this->rx_message_data_map_locks = new omp_lock_t[this->num_processors];
    for (uint32_t i=0; i < this->num_processors; i++) {
        omp_init_lock(&(this->rx_message_data_map_locks[i]));
        omp_init_lock(&(this->tx_message_data_map_locks[i]));
    }

	// initialize maps
	this->processor_id_to_tx_message_data_map = new std::map<uint32_t, std::vector<Message*>*>;
	this->processor_id_to_rx_message_data_map = new std::map<uint32_t, std::map<uint32_t, int>*>;
	for (uint32_t i=0; i < this->num_processors; i++) {
		std::vector<Message*>* tx_message_vec = new std::vector<Message*>;
		this->processor_id_to_tx_message_data_map->insert({i, tx_message_vec});
		std::map<uint32_t, int>* rx_message_map = new std::map<uint32_t, int>;
		this->processor_id_to_rx_message_data_map->insert({i, rx_message_map});
	}

	// create message sizes
	if (message_size_distribution == SIZE_RANDOM) {
		this->random_message_size_distribution_generator();
	}
	else if (message_size_distribution == SIZE_UNIFORM) {
		this->uniform_message_size_distribution_generator();
	}
	else {
    	raise(SIGTRAP);
    	assert(false);
    }

    // create messages
    if (message_node_distribution == NODE_RANDOM) {
    	random_message_node_distribution_generator();
    }
    else if (message_node_distribution == NODE_UNIFORM) {
    	uniform_message_node_distribution_generator();
    }
    else {
    	raise(SIGTRAP);
    	assert(false);
    }
}

void Message_Generator::update_tx_rx_data (Message* message) {
	uint32_t source_processor_id = message->source;
	uint32_t dest_processor_id = message->dest;
	uint32_t num_flits = message->num_flits;

	// add message to tx_data_message_map
	auto itr_tx = this->processor_id_to_tx_message_data_map->find(source_processor_id);
	std::vector<Message*>* tx_message_vec = itr_tx->second;

	omp_set_lock(&(this->tx_message_data_map_locks[source_processor_id]));
	tx_message_vec->push_back(message);
	this->num_messages_tx_by_processor[source_processor_id] += 1;
	omp_unset_lock(&(this->tx_message_data_map_locks[source_processor_id]));

	// increment flit count in rx_data_message_map
	auto itr_rx = this->processor_id_to_rx_message_data_map->find(dest_processor_id);
	std::map<uint32_t, int>* rx_message_map = itr_rx->second;

	omp_set_lock(&(this->rx_message_data_map_locks[dest_processor_id]));
	rx_message_map->insert({message->message_id, num_flits});
	this->num_messages_rx_by_processor[dest_processor_id] += 1;
	omp_unset_lock(&(this->rx_message_data_map_locks[dest_processor_id]));
}

void Message_Generator::random_message_size_distribution_generator () {
	uint32_t message_size_range = this->upper_message_size - this->lower_message_size;

    #pragma omp for schedule(static)
    for (uint32_t i=0; i < this->num_messages; i++) {
    	this->message_size_lst[i] = (rand() % message_size_range) + this->lower_message_size;
    }
}

void Message_Generator::uniform_message_size_distribution_generator () {
	uint32_t avg_message_size = (uint32_t)((this->upper_message_size + this->lower_message_size)/2);

    #pragma omp for schedule(static)
    for (uint32_t i=0; i < this->num_messages; i++) {
    	this->message_size_lst[i] = avg_message_size;
    }
}

void Message_Generator::random_message_node_distribution_generator () {
	#pragma omp for schedule(static)
	for (uint32_t i=0; i < this->num_messages; i++) {
		uint32_t message_size = this->message_size_lst[i];
		uint32_t source_processor_id = rand() % this->num_processors;
		// ensure dest processor is not same as source processor
		uint32_t dest_processor_id;
		do {
			dest_processor_id = rand() % this->num_processors;
		} while(source_processor_id == dest_processor_id);

		// create message
		Message* message = new Message(message_size, i, source_processor_id, dest_processor_id);
		this->update_tx_rx_data(message);
	}
}

void Message_Generator::uniform_message_node_distribution_generator () {

	// create vector with dest processor ids (each processor receives same number of messages)
	std::vector<uint32_t> dest_processor_id_vec;
	for (uint32_t i=0; i < this->num_messages; i++) {
		uint32_t dest_processor_id = i % this->num_processors;
		dest_processor_id_vec.push_back(dest_processor_id);
	}
	// shuffle vector so no trivial message distribution across nodes
	random_shuffle(dest_processor_id_vec.begin(), dest_processor_id_vec.end());

	#pragma omp for schedule(static)
	for (uint32_t i=0; i < this->num_messages; i++) {
		uint32_t message_size = this->message_size_lst[i];
		uint32_t source_processor_id = i % this->num_processors;
		uint32_t dest_processor_id = dest_processor_id_vec[i];
		// if dest processor id is same as source processor id, just randomly pick a new dest
		if (source_processor_id == dest_processor_id) {
			do {
				dest_processor_id = rand() % this->num_processors;
			} while(source_processor_id == dest_processor_id);
		}

		// create message
		Message* message = new Message(message_size, i, source_processor_id, dest_processor_id);
		this->update_tx_rx_data(message);
	}
}

std::vector<Message*>* Message_Generator::get_tx_message_data_vec (uint32_t processor_id) {
	auto itr = this->processor_id_to_tx_message_data_map->find(processor_id);
	return itr->second;
}

std::map<uint32_t, int>* Message_Generator::get_rx_message_data_map (uint32_t processor_id) {
	auto itr = this->processor_id_to_rx_message_data_map->find(processor_id);
	return itr->second;
}



