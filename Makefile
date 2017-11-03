# Config build structure ##########################################
BIN = mcast-iface-tool
SRC = $(wildcard *.cpp)
OBJS = $(SRC:.cpp=.o)
# Config build structure end ######################################

# Compiler flags -------------------------------------------------
CC =g++
DEBUG = -Os
CFLAGS = -rdynamic -Wall -Wextra -Werror $(DEBUG)
IFLAGS = -I. 
LDFLAGS = -pthread
ARCHFLAGS = 
# Compiler flags ends ---------------------------------------------
.PHONY: all

all: clean
	$(MAKE) $(BIN)
	
clean:
	-rm -f $(OBJS) $(BIN)

# Build code #######################################
%.o:%.cpp
	$(CC) -c $(CFLAGS) $(IFLAGS) $(ARCHFLAGS) $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(IFLAGS) $(ARCHFLAGS) -o $@

