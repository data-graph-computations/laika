CC = gcc
CCFLAGS = -ansi-alias -O3 -Wall
LDFLAGS =

.PHONY: all clean

SOURCES = reorder.cpp

all: reorder

reorder: $(SOURCES)
	$(CC) $(CCFLAGS) $(LDFLAGS) -o reorder.out $(SOURCES)

clean:
	rm -f *~ *.o *.out
