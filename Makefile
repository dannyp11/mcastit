# Compiler flags -------------------------------------------------
CXX ?=g++
DEBUG = -Os
CFLAGS = -rdynamic -Wall -Wextra -Werror $(DEBUG)
IFLAGS = -I. 
LDFLAGS = -pthread
ARCHFLAGS = -m32
# Compiler flags ends ---------------------------------------------

# Config build structure ##########################################
BIN = mcastit
SRC = $(wildcard *.cpp)
OBJS = $(SRC:.cpp=.o)
# Config build structure end ######################################

.PHONY: all

all: $(BIN)
	
clean:
	-rm -f $(OBJS) $(BIN)

# Build code #######################################
%.o:%.cpp
	$(CXX) -c $(CFLAGS) $(IFLAGS) $(ARCHFLAGS) $< -o $@

$(BIN): $(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJS) $(IFLAGS) $(ARCHFLAGS) -o $@

