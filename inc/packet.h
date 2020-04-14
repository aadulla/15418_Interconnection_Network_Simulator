#include "flit.h"

extern uint32_t flit_width;

class Packet {

private:
	Head_Flit head;
	Data_Flit[] payload;
	Tail_Flit tail;

public:
	Packet(uint32_t dest, uint32_t packet_num, uint32_t packet_size);

}