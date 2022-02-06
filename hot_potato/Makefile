CC = g++
CFLAGS = -std=gnu++11 -pedantic -g -Wall

all: ringmaster player

ringmaster: ringmaster.cpp potato.hpp
	$(CC) $(CFLAGS) -o $@ ringmaster.cpp
player: player.cpp potato.hpp
	$(CC) $(CFLAGHS) -o $@ player.cpp

clean:
	rm -rf ringmaster player *.hpp~ *.cpp~ *.o
