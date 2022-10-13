CC = g++
CFLAGS = -g
BINS = tracker peer

all: $(BINS)

%: %.cpp
	$(CC) $(CFLAGS) -o bin/$@ $^

clean:
	rm -rf $(BINS)