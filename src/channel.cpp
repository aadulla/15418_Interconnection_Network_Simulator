#include <string>
#include <pthread.h>

#include "router.h"
#include "channel.h"

Channel::Channel (Router* source, Router* dest, string direction) {
	this.source = source;
	this.dest = dest;
	this.direction = direction;
}

bool Channel::transmit_flit () {
	bool is_successful = false;
	Buffer dest_buffer = dest->get_buffer(&this);

	if (dest_buffer->get_occupied_status(&this) == THIS_OCCUPIED) {

		if (dest_buffer->get_capacity_status(&this) == NOT_FULL) {
			is_successful = true;
			Buffer source_buffer = source->get_buffer(&this);

			pthread_mutex_lock(source_buffer->buffer_mutex);
			this.flit = source_buffer->remove_flit();
			pthread_mutex_unlock(source_buffer->buffer_mutex);

			pthread_mutex_lock(dest_buffer->buffer_mutex);
			dest_buffer->insert_flit(this.flit)
			pthread_mutex_unlock(dest_buffer->buffer_mutex);
		}
	}
	
	return is_successful;
}