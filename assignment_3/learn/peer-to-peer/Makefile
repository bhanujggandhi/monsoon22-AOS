CC = g++
CFLAGS = -g
BINS = tracker peer peer2

all: $(BINS)

%: %.cpp
	$(CC) $(CFLAGS) -o bin/$@ $^

clean:
	rm -rf bin/peer
	rm -rf bin/tracker
	rm -rf bin/peer2