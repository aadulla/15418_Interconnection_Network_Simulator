#ifndef BUFFER_H
#define BUFFER_H

#include <queue>
#include <bitset>
#include <pthread.h>

#include "flit.h"

enum BUFFER_CAPACITY_STATUS { NOT_FULL, FULL };

class Buffer {

private:
	uint32_t max_flits;
	std::queue<Flit*>* queue;
	BUFFER_STATUS capacity_status;
	pthread_mutex_t buffer_mutex;

public:
	Buffer(uint32_t max_flits);
	bool insert_flit(Flit* flit);
	Flit* remove_flit();

}

#endif /* BUFFER_H */