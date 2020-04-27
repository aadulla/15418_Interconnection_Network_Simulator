#include <map>
#include <cassert>
#include <signal.h>

// #include "routing_algorithms.h"
// #include "node.h"
#include "buffer.h"
#include "flit.h"

/* flow control algorithms */

// only return true if the entire packet is inside the buffer
bool store_forward_flow_control (Flit* flit, Buffer* buffer) {
	// if this is not a head flit, then this means that the head flit of the packet
	// has already been transmitted, so we can transmit this flit
	if (flit->type != HEAD) return true;

	// this is a head flit so need to check if the corresponding tail flit is inside the buffer
	for (auto itr=buffer->begin(); itr != buffer->end(); itr++) {
		Flit* test_flit = *itr;
		if (test_flit->message_id == flit->message_id && 
			test_flit->packet_id == flit->packet_id &&
			test_flit->type == TAIL) 
			return true;
	}

	// entire packet is no queued up in buffer
	return false;

}

// always return true because transmission does not have to wait on anything
bool cut_through_flow_control (Flit* flit, Buffer* buffer) {
	return true;
}