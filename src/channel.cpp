#include <stdint.h>
#include <string>
#include <cassert>
#include <signal.h>

#include "node.h"
#include "channel.h"
#include "flit.h"

uint32_t global_channel_id;

Channel::Channel (Node* source, Node* dest) {
	this->channel_id = global_channel_id++;
	this->source = source;
	this->dest = dest;
	this->transmission_state = new Transmission_State;
	this->reset_transmission_state();
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

bool Channel::is_unlocked_for_flit (Flit* flit) {
	bool is_unlocked = this->transmission_state->lock_status == UNLOCKED;
	bool neq_message_id = this->transmission_state->message_id != flit->message_id;
	bool neq_packet_id = this->transmission_state->packet_id != flit->packet_id;
	return is_unlocked && (neq_message_id || neq_packet_id);
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

	// update distance if transmitting a head flit
	FLIT_TYPE flit_type = flit_to_transmit->type;
	if (flit_type == HEAD) {
		Head_Flit* head_flit = (Head_Flit*)flit_to_transmit;
		head_flit->increment_distance();
	}

	return flit_type;
}

void Channel::fail_transmission () {
	this->transmission_state->transmission_status = FAIL;
}

bool Channel::is_failed_transmission () {
	return this->transmission_state->transmission_status == FAIL;
}

void Channel::clear_transmission_status () {
	this->transmission_state->transmission_status = CLEAR;
}
