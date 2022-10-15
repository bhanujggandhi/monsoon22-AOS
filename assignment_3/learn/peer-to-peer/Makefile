CC = g++
CFLAGS = -lcrypto
BINS = peer peer2 peer3

all: tracker $(BINS)

tracker: tracker.cpp
	$(CC) -o $@ $^

%: %.cpp
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm -rf tracker
	rm -rf peer
	rm -rf peer2
	rm -rf peer3