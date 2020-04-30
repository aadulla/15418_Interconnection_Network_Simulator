#include <stdint.h>
#include <string>
#include <cassert>
#include <signal.h>

#include "node.h"
#include "channel.h"
#include "buffer.h"
#include "flit.h"

uint32_t global_channel_id;

Channel::Channel (Node* source, Node* dest) {
	this->channel_id = global_channel_id++;
	this->source = source;
	this->dest = dest;
	this->transmission_state = new Transmission_State;
	this->reset_transmission_state();
	this->transmission_state->flit_type = HEAD;
	this->buffer_lst = NULL;
	this->num_buffers = 0;
}

void Channel::init_buffer_lst (Buffer** buffer_lst, uint32_t num_buffers) {
	this->buffer_lst = buffer_lst;
	this->num_buffers = num_buffers;
}

bool Channel::is_dest_buffer_reserved_for_flit (Flit* flit) {
	bool is_reserved = false;
	for (uint32_t i=0; i < this->num_buffers; i++) {
		Buffer* buffer = this->buffer_lst[i];
		if (buffer->is_reserved_for_flit(flit->message_id, flit->packet_id)) {
			is_reserved = true;
			break;
		}
	}
	return is_reserved;
}

bool Channel::is_dest_buffer_reserved_for_flit_and_full (Flit* flit) {
	bool is_reserved = false;
	bool is_full = false;
	for (uint32_t i=0; i < this->num_buffers; i++) {
		Buffer* buffer = this->buffer_lst[i];
		if (buffer->is_reserved_for_flit(flit->message_id, flit->packet_id)) {
			is_reserved = true;
			is_full = buffer->is_full();
			break;
		}
	}
	return is_reserved && is_full;
}

bool Channel::is_dest_buffer_unreserved () {
	bool is_unreserved = false;
	for (uint32_t i=0; i < this->num_buffers; i++) {
		Buffer* buffer = this->buffer_lst[i];
		if (buffer->is_unreserved()) {
			if (buffer->is_empty() || buffer->is_not_full()) {
				is_unreserved = true;
				break;
			}
		}
	}
	return is_unreserved;

}

void Channel::unlock () {
	this->transmission_state->lock_status = UNLOCKED;
}

void Channel::lock () {
	this->transmission_state->lock_status = LOCKED;
}

bool Channel::is_locked_for_flit (Flit* flit) {
	bool is_locked = this->transmission_state->lock_status == LOCKED;
	bool eq_message_id = this->transmission_state->message_id == flit->message_id;
	bool eq_packet_id = this->transmission_state->packet_id == flit->packet_id;
	return is_locked && eq_message_id && eq_packet_id;
}

bool Channel::is_unlocked () {
	bool is_unlocked = this->transmission_state->lock_status == UNLOCKED;
	return is_unlocked;
}

bool Channel::is_open_for_transmission () {
	return this->transmission_state->flit_status == UNASSIGNED;
}

bool Channel::is_closed_for_transmission () {
	return this->transmission_state->flit_status == ASSIGNED;
}

void Channel::reset_transmission_state () {
	this->transmission_state->tx_buffer = NULL;
	this->transmission_state->flit_status = UNASSIGNED;
	this->transmission_state->lock_status = UNLOCKED;
	this->transmission_state->transmission_status = CLEAR;
	this->transmission_state->packet_id = (uint32_t)-1;
	this->transmission_state->message_id = (uint32_t)-1;
}

void Channel::propose_transmission (Buffer* tx_buffer) {
	Flit* flit_to_transmit = tx_buffer->peek_flit();
	// if channel is locked, assert that the flit info matches with previous transmission state
	if (this->transmission_state->lock_status == LOCKED) {
		assert(this->transmission_state->message_id == flit_to_transmit->message_id);
		assert(this->transmission_state->packet_id == flit_to_transmit->packet_id);
	}
	// ensure there is not a flit already placed in this channel
	assert(this->transmission_state->flit_status == UNASSIGNED);
	// update transmission state
	this->transmission_state->tx_buffer = tx_buffer;
	this->transmission_state->flit_status = ASSIGNED;
	this->transmission_state->message_id = flit_to_transmit->message_id;
	this->transmission_state->packet_id = flit_to_transmit->packet_id;
}

FLIT_TYPE Channel::execute_transmission (Buffer* rx_buffer) {
	// pull flit from tx buffer
	Flit* flit_to_transmit = this->transmission_state->tx_buffer->remove_flit();
	// push flit to rx buffer
	rx_buffer->insert_flit(flit_to_transmit);
	// set flit status to EMPTY
	this->transmission_state->flit_status = UNASSIGNED;

	FLIT_TYPE flit_type = flit_to_transmit->type;

	// update distance if transmitting a head flit
	if (flit_type == HEAD) {
		Head_Flit* head_flit = (Head_Flit*)flit_to_transmit;
		head_flit->increment_distance();
	}

	// erase cached routing if transmitting a tail flit
	if (flit_type == TAIL && (this->source->type == ROUTER || this->source->type == PROCESSOR_ROUTER)) {
		Router* router = (Router*)this->source;
		router->erase_cached_routing(flit_to_transmit->message_id, flit_to_transmit->packet_id);
	}

	this->transmission_state->transmission_status = SUCCESS;
	this->transmission_state->flit_type = flit_type;

	return flit_type;
}

FLIT_TYPE Channel::get_transmitted_flit_type () {
	return this->transmission_state->flit_type;
}

void Channel::fail_transmission () {
	this->transmission_state->transmission_status = FAIL;
	// this->transmission_state->flit_status = UNASSIGNED;
}

void Channel::clear_transmission_status () {
	this->transmission_state->transmission_status = CLEAR;
	// this->transmission_state->flit_status = UNASSIGNED;
}

bool Channel::is_failed_transmission () {
	bool is_failed = this->transmission_state->transmission_status == FAIL;
	this->clear_transmission_status();
	return is_failed;
}

bool Channel::is_successful_transmission () {
	bool is_successful = this->transmission_state->transmission_status == SUCCESS;
	this->clear_transmission_status();
	return is_successful;

}