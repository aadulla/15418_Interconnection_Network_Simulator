#ifndef FLIT_H
#define FLIT_H

#include <stdint.h>

typedef enum { HEAD, DATA, TAIL } FLIT_TYPE;

class Flit {

public:
	uint32_t flit_id;
	uint32_t packet_id;
	uint32_t message_id;
	uint32_t num_packets;
	FLIT_TYPE type;

public:
	Flit(uint32_t flit_id, uint32_t packet_id, uint32_t message_id, uint32_t num_packets, FLIT_TYPE type);

};

class Border_Flit : public Flit {

public:
	uint32_t source;
	uint32_t dest;

public:
	Border_Flit(uint32_t flit_id, uint32_t packet_id, uint32_t message_id, uint32_t num_packets, FLIT_TYPE type, uint32_t source, uint32_t dest);

};

class Head_Flit : public Border_Flit {

public:
	uint32_t distance;
	Head_Flit(uint32_t flit_id, uint32_t packet_id, uint32_t message_id, uint32_t num_packets, uint32_t source, uint32_t dest);
	void increment_distance();

};

class Tail_Flit : public Border_Flit {

public:
	Tail_Flit(uint32_t flit_id, uint32_t packet_id, uint32_t message_id, uint32_t num_packets, uint32_t source, uint32_t dest);

};

class Data_Flit : public Flit {

public:
	Data_Flit(uint32_t flit_id, uint32_t packet_id, uint32_t message_id, uint32_t num_packets);

};

#endif /* FLIT_H */