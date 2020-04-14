#ifndef CHANNEL_H
#define CHANNEL_H

#include <bitset>
#include <string>

#include "router.h"

extern uint32_t flit_width;

class Channel {
private:
	Router* source;
	Router* dest;
	string direction;
	Flit* flit;

public:
	Channel (Router* source, Router* dest, string direction);
	void transmit_flit ();
}

