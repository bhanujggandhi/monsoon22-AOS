CC = g++
CFLAGS = -g
BINS = tracker peer peer2

all: $(BINS)

tracker: tracker.cpp
	$(CC) $(CFLAGS) -o $@ tracker.cpp

peer: peer.cpp
	$(CC) $(CFLAGS) -o $@ peer.cpp

peer2: peer2.cpp
	$(CC) $(CFLAGS) -o $@ peer2.cpp


clean:
	rm -rf peer
	rm -rf tracker
	rm -rf peer2