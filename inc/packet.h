#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#include "flit.h"

class Packet {

private:
	uint32_t source;
	uint32_t dest;
	uint32_t packet_id;
	uint32_t message_id;
	uint32_t num_packets;

public:
	Head_Flit* head;
	Data_Flit** payload;
	Tail_Flit* tail;
	
	Packet(uint32_t packet_id, uint32_t message_id, uint32_t num_packets, uint32_t source, uint32_t dest);

};

#endif /* PACKET_H */