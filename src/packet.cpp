#include <stdint.h>

#include "packet.h"
#include "flit.h"

extern uint32_t num_data_flits_per_packet;

Packet::Packet(uint32_t packet_id, uint32_t message_id, uint32_t num_packets, uint32_t source, uint32_t dest) {
	this->source = source;
	this->dest = dest;
	this->packet_id = packet_id;
	this->message_id = message_id;
	this->num_packets = num_packets;

	uint32_t flit_id = 0;
	this->head = new Head_Flit(flit_id++, this->packet_id, this->message_id, this->num_packets, this->source, this->dest);
	this->payload = new Data_Flit*[num_data_flits_per_packet];
	for (uint32_t i=0; i < num_data_flits_per_packet; i++) {
		this->payload[i] = new Data_Flit(flit_id++, this->packet_id, this->message_id, this->num_packets);
	}
	this->tail = new Tail_Flit(flit_id++, this->packet_id, this->message_id, this->num_packets, this->source, this->dest);
}