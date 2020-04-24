#ifndef FLOW_CONTROL_ALGORITHMS_H
#define FLOW_CONTROL_ALGORITHMS_H

typedef void (*TX_Flow_Control_Func)(void*);
typedef void (*RX_Flow_Control_Func)(void*);

void packet_tx_store_forward (void* router_ptr);
void packet_rx_store_forward (void* router_ptr);

#endif /* FLOW_CONTROL_ALGORITHMS_H */