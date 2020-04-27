#ifndef FLOW_CONTROL_ALGORITHMS_H
#define FLOW_CONTROL_ALGORITHMS_H

typedef enum { PACKET, FLIT } FLOW_CONTROL_GRANULARITY;

class Router;
class Buffer;
class Flit;

typedef bool (*Flow_Control_Func)(Flit*, Buffer*);

/* flow control algorithms */
bool store_forward_flow_control(Flit* flit, Buffer* buffer);
bool cut_through_flow_control(Flit* flit, Buffer* buffer);

// typedef void (*TX_Flow_Control_Func)(void*);
// typedef void (*RX_Flow_Control_Func)(void*);

// void packet_tx_store_forward (void* router_ptr);
// void packet_rx_store_forward (void* router_ptr);

// void packet_tx_cut_through (void* router_ptr);
// void packet_rx_cut_through (void* router_ptr);

// void flit_tx_wormhole (void* router_ptr);
// void flit_rx_wormhole (void* router_ptr);

#endif /* FLOW_CONTROL_ALGORITHMS_H */