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

#endif /* FLOW_CONTROL_ALGORITHMS_H */