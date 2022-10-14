CC = g++
CFLAGS = -g
BINS = tracker peer peer2 peer3

all: $(BINS)

%: %.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf peer
	rm -rf tracker
	rm -rf peer2
	rm -rf peer3