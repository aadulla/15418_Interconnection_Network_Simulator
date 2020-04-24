#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#include "packet.h"

typedef struct _Message_Transmission_Info {
	float avg_packet_distance;
	uint32_t tx_processor_id;
	int tx_time;
	uint32_t rx_processor_id;
	int rx_time;
} Message_Transmission_Info;

class Message {

public:
	uint32_t message_id;
	uint32_t size;
	uint32_t num_packets;
	uint32_t num_flits;
	uint32_t source;
	uint32_t dest;
	Packet** packet_lst;
	
	Message(uint32_t size, uint32_t message_id, uint32_t source, uint32_t dest);
	

};

#endif /* MESSAGE_H */