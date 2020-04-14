#include <queue>
#include <cassert>
#include <pthread.h>

#include "buffer.h"
#include "flit.h"

Buffer::Buffer(uint32_t max_flits) {
	this.queue = new std::queue<Flit*>;
	this.max_flits = max_flits;
	this.status = NOT_FULL;
	pthread_mutex_init(&this.buffer_mutex);
}

bool Buffer::insert_flit(Flit* flit) {
	bool is_successful = false;
	if (this.status == NOT_FULL) {
		is_successful = true;
		this.queue->push(flit);
		this.state = this.queue->size() == this.max_flits ? FULL : NOT_FULL;
	}
	return is_sucessful;
}

Flit* Buffer::remove_flit() {
	assert(this.queue->size() > 0);
	Flit* flit = this.queue->pop();
	this.state = NOT_FULL;
	return flit;
}