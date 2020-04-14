#ifndef ROUTER_H
#define ROUTER_H

#include <set>
#include <map>

#include "channel.h"

enum BUFFER_OCCUPIED_STATUS { THIS_OCCUPIED, OTHER_OCCUPIED};

typedef struct _io_channel {
	Channel* input_channel;
	Channel* output_channel;
} io_channel;

class Router {

private:
	uint32_t id;
	uint32_t num_io_channels;
	std::map<Channel*, Buffer*>* channel_buffer_map;
	Channel* buffer_occupier;

public:
	Router(uint32_t id, uint32_t num_io_channels);
	void initialize_channel_buffer_connection(Channel* channel, Buffer* buffer, string direction);
	Buffer* get_buffer(Channel* channel);
	send_flit()
}


#endif /* ROUTER_H */