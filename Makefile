APP_NAME = main

OBJDIR = obj
SRCDIR = src
INCDIR = inc

CXX = g++ -Wall -g 
CXXFLAGS = -I$(INCDIR)

_INCS = main.h
INCS = $(patsubst %,$(INCDIR)/%,$(_INCS))

_OBJS = main.o 
OBJS = $(patsubst %,$(OBJDIR)/%,$(_OBJS))

_SRCS = main.cpp
SRCS = $(patsubst %,$(SRCDIR)/%,$(_SRCS))

.PHONY: clean

clean:
	rm -rf $(OBJS) $(APP_NAME)

# all objs/*.o files depend on src/*.cpp and inc/*.h files
$(OBJS): $(SRCS) $(INCS)
# g++ -c -o obj/*.o *src/*.cpp -(g++ flags)
		$(CXX) -c -o $@ $< $(CXXFLAGS)

# create executable main
$(APP_NAME): $(OBJS)
# g++ -(g++ flags) -o main obj/*.o
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS)