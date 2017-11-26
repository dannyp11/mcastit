# Compiler flags -------------------------------------------------
CXX ?=g++
DEBUG = -Os
CFLAGS = -Wall -Wextra -Werror $(DEBUG)
IFLAGS = -I. 
LDFLAGS = -pthread -rdynamic
ARCHFLAGS = 
# Compiler flags ends ---------------------------------------------

# Install data ----------------------------------------------------
DESTDIR ?=/usr/local
INSTALLDIR_BIN=$(DESTDIR)/bin/
# -----------------------------------------------------------------

# Config build structure ##########################################
BIN = mcastit
SRC = $(wildcard *.cpp)
OBJS = $(SRC:.cpp=.o)
# Config build structure end ######################################

.PHONY: all

all: $(BIN)
	
clean:
	-rm -f $(OBJS) $(BIN)

install: all
	mkdir -p $(INSTALLDIR_BIN)
	install $(BIN) $(INSTALLDIR_BIN)

uninstall:
	rm -f $(INSTALLDIR_BIN)/$(BIN)

# Build code #######################################
%.o:%.cpp
	$(CXX) -c $(CFLAGS) $(IFLAGS) $(ARCHFLAGS) $< -o $@

$(BIN): $(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJS) $(IFLAGS) $(ARCHFLAGS) -o $@

