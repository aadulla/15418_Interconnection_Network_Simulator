#include "packet.h"

class Message {

private:
	uint32_t size;
	uint32_t source;
	uint32_t dest;
	Packet[] packet_lst;

public:
	Message(uint32_t size, uint32_t source, uint32_t dest);
	

}