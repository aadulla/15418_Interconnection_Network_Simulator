#include <stdint.h>
#include <deque>
#include <cassert>
#include <signal.h>

#include "buffer.h"
#include "flit.h"

Buffer::Buffer (uint32_t max_capacity) {
	this->queue = new std::deque<Flit*>;
	this->max_capacity = max_capacity;
	this->capacity_status = EMPTY;
	this->reserved_status = UNRESERVED;
	this->reserved_message_id = (uint32_t)-1;
	this->reserved_packet_id = (uint32_t)-1;
}

void Buffer::update_capacity_status () {
	if (this->queue->empty()) {
		this->capacity_status = EMPTY;
	}
	else if (this->queue->size() == this->max_capacity) {
		this->capacity_status = FULL;
	}
	else {
		this->capacity_status = NOT_FULL;
	}
}

void Buffer::reserve_buffer (uint32_t message_id, uint32_t packet_id) {
	assert(this->reserved_status != RESERVED);
	this->reserved_status = RESERVED;
	this->reserved_message_id = message_id;
	this->reserved_packet_id = packet_id;
}

void Buffer::unreserve_buffer () {
	assert(this->reserved_status != UNRESERVED);
	this->reserved_status = UNRESERVED;
	this->reserved_message_id = (uint32_t)-1;
	this->reserved_packet_id = (uint32_t)-1;
}

bool Buffer::insert_flit (Flit* flit) {
	bool is_successful = false;
	if (this->capacity_status != FULL) {
		is_successful = true;
		this->queue->push_back(flit);
	}
	this->update_capacity_status();
	return is_successful;
}

Flit* Buffer::remove_flit () {
	assert(!this->queue->empty());
	Flit* flit = this->queue->front();
	this->queue->pop_front();
	this->update_capacity_status();
	return flit;
}

Flit* Buffer::peek_flit () {
	Flit* flit = this->queue->front();
	return flit;
}

bool Buffer::is_full () {
	this->update_capacity_status();
	return this->capacity_status == FULL;
}

bool Buffer::is_not_full () {
	this->update_capacity_status();
	return this->capacity_status == NOT_FULL;
}

bool Buffer::is_empty () {
	this->update_capacity_status();
	return this->capacity_status == EMPTY;
}

uint32_t Buffer::occupied_size () {
	return (uint32_t)this->queue->size();
}

uint32_t Buffer::total_size () {
	return this->max_capacity;
}

bool Buffer::is_reserved_for_flit (uint32_t message_id, uint32_t packet_id) {
	bool is_reserved = this->reserved_status == RESERVED;
	bool is_matching = this->reserved_message_id == message_id && this->reserved_packet_id == packet_id;
	return is_reserved && is_matching;
}

bool Buffer::is_unreserved () {
	bool is_unreserved = this->reserved_status == UNRESERVED;
	return is_unreserved;
}