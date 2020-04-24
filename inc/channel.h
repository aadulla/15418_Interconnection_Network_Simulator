#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdint.h>

#include "node.h"
#include "buffer.h"

class Node;

typedef enum { ASSIGNED, UNASSIGNED } FLIT_STATUS;
typedef enum { LOCKED, UNLOCKED } LOCK_STATUS;
typedef enum { CLEAR, SUCCESS, FAIL } TRANSMISSION_STATUS;

typedef struct _Transmission_State {
	Buffer* tx_buffer;
	FLIT_STATUS flit_status;
	LOCK_STATUS lock_status;
	TRANSMISSION_STATUS transmission_status;
	uint32_t packet_id;
	uint32_t message_id;
} Transmission_State;


class Channel {

private:
	Node* source;
	Node* dest;

public:
	uint32_t channel_id;
	Transmission_State* transmission_state; 
	
	Channel (Node* source, Node* dest);
	void unlock();
	void lock();
	bool is_locked_for_flit(Flit* flit);
	bool is_unlocked_for_flit(Flit* flit);
	bool is_open_for_transmission();
	bool is_closed_for_transmission();
	void reset_transmission_state();
	void propose_transmission(Buffer* tx_buffer);
	FLIT_TYPE execute_transmission(Buffer* rx_buffer);
	void fail_transmission();
	bool is_failed_transmission();
	void clear_transmission_status();
};

#endif /* CHANNEL_H */