#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <deque>
#include <iterator>

#include "flit.h"

typedef enum { EMPTY, NOT_FULL, FULL } BUFFER_STATUS;

class Buffer {

private:
	BUFFER_STATUS status;
	typedef typename std::deque<Flit*>::iterator iterator;

public:
	std::deque<Flit*>* queue;
	uint32_t max_capacity;
	Buffer(uint32_t max_capacity);
	void update_status();
	bool insert_flit(Flit* flit);
	Flit* remove_flit();
	Flit* peek_flit();
	uint32_t occupied_size();
	uint32_t total_size();
	bool is_not_full();
	bool is_empty();


	iterator begin() { return queue->begin(); }
	iterator end() { return queue->end(); }

};

#endif /* BUFFER_H */