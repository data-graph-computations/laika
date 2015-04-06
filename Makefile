CC = gcc
CCFLAGS = -std=c++11 -ansi-alias -O3 -Wall
LDFLAGS =

.PHONY: all clean lint

HEADERS = common.h
SOURCES = reorder.cpp

TEST = 0
DEBUG = 0

all: lint reorder

set-debug: DEBUG=1

set-test: TEST=1

lint:
	cpplint.py *.cpp *.h

reorder: $(SOURCES)
	$(CC) $(CCFLAGS) $(LDFLAGS) -DTEST=$(TEST) -DDEBUG=$(DEBUG) -o reorder.out $(SOURCES)

clean:
	rm -f *~ *.o *.out
