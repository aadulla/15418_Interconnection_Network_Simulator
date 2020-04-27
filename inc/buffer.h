#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <deque>
#include <iterator>

#include "flit.h"

typedef enum { EMPTY, NOT_FULL, FULL } BUFFER_CAPACITY_STATUS;
typedef enum { UNRESERVED, RESERVED } BUFFER_RESERVED_STATUS;

class Buffer {

private:
	BUFFER_CAPACITY_STATUS capacity_status;
	BUFFER_RESERVED_STATUS reserved_status;
	uint32_t reserved_message_id;
	uint32_t reserved_packet_id;
	typedef typename std::deque<Flit*>::iterator iterator;

public:
	std::deque<Flit*>* queue;
	uint32_t max_capacity;
	Buffer(uint32_t max_capacity);
	void update_capacity_status();
	void reserve_buffer(uint32_t message_id, uint32_t packet_id);
	void unreserve_buffer();
	bool insert_flit(Flit* flit);
	Flit* remove_flit();
	Flit* peek_flit();
	uint32_t occupied_size();
	uint32_t total_size();
	bool is_full();
	bool is_not_full();
	bool is_empty();
	bool is_reserved_for_flit(uint32_t message_id, uint32_t packet_id);
	bool is_unreserved();


	iterator begin() { return queue->begin(); }
	iterator end() { return queue->end(); }

};

#endif /* BUFFER_H */