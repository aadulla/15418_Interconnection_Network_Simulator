#include <stdint.h>
#include <signal.h>

#include "message.h"
#include "packet.h"

extern uint32_t packet_width;
extern uint32_t num_data_flits_per_packet;
extern Message_Transmission_Info** global_message_transmission_info;

Message::Message(uint32_t size, uint32_t message_id, uint32_t source, uint32_t dest) {
	this->size = size;
	this->num_packets = this->size / packet_width;
	this->num_flits = (uint32_t)(this->num_packets * (num_data_flits_per_packet + 2));
	this->source = source;
	this->dest = dest;
	this->message_id = message_id;

	global_message_transmission_info[this->message_id]->avg_packet_distance = 0.0;
	global_message_transmission_info[this->message_id]->latency = 0;
	global_message_transmission_info[this->message_id]->size = size;
	global_message_transmission_info[this->message_id]->tx_processor_id = this->source;
	global_message_transmission_info[this->message_id]->tx_time = -15418;
	global_message_transmission_info[this->message_id]->rx_processor_id = this->dest;
	global_message_transmission_info[this->message_id]->rx_time = -15418;

	
	this->packet_lst = new Packet*[this->num_packets];
	for (uint32_t i=0; i < this->num_packets; i++) {
		Packet* new_packet = new Packet(i, this->message_id, this->num_packets, this->source, this->dest);
		if (new_packet == nullptr) raise(SIGTRAP);
		this->packet_lst[i] = new_packet;
	}
}

