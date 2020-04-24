#include <stdint.h>

#include "flit.h"

Flit::Flit(uint32_t flit_id, uint32_t packet_id, uint32_t message_id, uint32_t num_packets, FLIT_TYPE type) {
	this->flit_id = flit_id;
	this->packet_id = packet_id;
	this->message_id = message_id;
	this->num_packets = num_packets;
	this->type = type;
}

Border_Flit::Border_Flit(uint32_t flit_id, 
						 uint32_t packet_id, 
						 uint32_t message_id, 
						 uint32_t num_packets,
						 FLIT_TYPE type, 
						 uint32_t source, 
						 uint32_t dest) :
Flit(flit_id, packet_id, message_id, num_packets, type) {
	this->type = type;
	this->source = source;
	this->dest = dest;
}

Head_Flit::Head_Flit(uint32_t flit_id, 
					 uint32_t packet_id, 
					 uint32_t message_id, 
					 uint32_t num_packets,
					 uint32_t source, 
					 uint32_t dest) : 
Border_Flit(flit_id, packet_id, message_id, num_packets, HEAD, source, dest) {}

void Head_Flit::increment_distance() {
	this->distance += 1;
}

Tail_Flit::Tail_Flit(uint32_t flit_id, 
					 uint32_t packet_id, 
					 uint32_t message_id, 
					 uint32_t num_packets,
					 uint32_t source, 
					 uint32_t dest) : 
Border_Flit(flit_id, packet_id, message_id, num_packets, TAIL, source, dest) {}

Data_Flit::Data_Flit(uint32_t flit_id, 
					 uint32_t packet_id, 
					 uint32_t message_id,
					 uint32_t num_packets) :
Flit(flit_id, packet_id, message_id, num_packets, DATA) {}

