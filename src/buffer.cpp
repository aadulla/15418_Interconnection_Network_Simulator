#include <stdint.h>
#include <deque>
#include <cassert>
#include <signal.h>

#include "buffer.h"
#include "flit.h"

Buffer::Buffer(uint32_t max_capacity) {
	this->queue = new std::deque<Flit*>;
	this->max_capacity = max_capacity;
	this->status = EMPTY;
}

void Buffer::update_status() {
	if (this->queue->empty()) {
		this->status = EMPTY;
	}
	else if (this->queue->size() == this->max_capacity) {
		this->status = FULL;
	}
	else {
		this->status = NOT_FULL;
	}
}

bool Buffer::insert_flit(Flit* flit) {
	bool is_successful = false;
	if (this->status != FULL) {
		is_successful = true;
		this->queue->push_back(flit);
	}
	this->update_status();
	return is_successful;
}

Flit* Buffer::remove_flit() {
	assert(!this->queue->empty());
	Flit* flit = this->queue->front();
	this->queue->pop_front();
	this->update_status();
	return flit;
}

Flit* Buffer::peek_flit() {
	Flit* flit = this->queue->front();
	return flit;
}

bool Buffer::is_not_full () {
	this->update_status();
	return this->status == NOT_FULL;
}

bool Buffer::is_empty () {
	this->update_status();
	return this->status == EMPTY;
}

uint32_t Buffer::occupied_size() {
	return (uint32_t)this->queue->size();
}

uint32_t Buffer::total_size() {
	return this->max_capacity;
}