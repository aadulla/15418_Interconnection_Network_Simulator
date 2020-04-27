SRCDIR = src
INCDIR = inc

CXX = g++ -Wall -g -std=c++11
CXXFLAGS = -I$(INCDIR)
OMP = -fopenmp -DOMP

_INCS = buffer.h channel.h config_parser.h CycleTimer.h flit.h flow_control_algorithms.h message.h message_generator.h network.h node.h packet.h routing_algorithms.h simulator.h
INCS = $(patsubst %,$(INCDIR)/%,$(_INCS))

_SRCS = main.cpp buffer.cpp channel.cpp config_parser.cpp flit.cpp flow_control_algorithms.cpp message.cpp message_generator.cpp network.cpp node.cpp packet.cpp routing_algorithms.cpp simulator.cpp
SRCS = $(patsubst %,$(SRCDIR)/%,$(_SRCS))

# $(info $$INCS is [${INCS}])
# $(info $$SRCS is [${SRCS}])

.PHONY: clean

clean:
	rm -rf $(OBJS) $(APP_NAME)

# seq: $(SRCS) $(SEQ_SRCS) $(INCS) $(SEQ_INCS)
# 		$(CXX) $(CXXFLAGS) $(SRCS) $(SEQ_SRCS) -o $@

main: $(SRCS) $(INCS)
		$(CXX) $(CXXFLAGS) $(OMP) $(SRCS) -o $@