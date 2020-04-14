#include <bitset>

extern uint32_t flit_width;

class Flit {

}

class Head_Flit : public Flit {

private:
	uint32_t dest;
	uint32_t packet_num;
	uint32_t num_flits_in_packet

public:
	Head_Flit(uint32_t dest, uint32_t packet_num, num_flits_in_packet);
	uint32_t get_dest();
	uint32_t get_packet_num();
	uint32_t get_num_flits_in_packet();

}

class Data_Flit : public Flit {

private:
	uint32_t flit_num;

public:
	Data_Flit(uint32_t flit_num);
	uint32_t get_flit_num();

}

class Tail_Flit : public Flit {

private:
	uint32_t dest;
	uint32_t packet_num;
	uint32_t num_flits_in_packet;

public:
	Tail_Flit(uint32_t dest, uint32_t packet_num, uint32_t num_flits_in_packet);
	uint32_t get_dest();
	uint32_t get_packet_num();
	uint32_t get_num_flits_in_packet();

}