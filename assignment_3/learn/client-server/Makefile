CC = g++
CFLAGS = -g
BINS = server client

all: $(BINS)

%: %.cpp
	$(CC) $(CFLAGS) -o bin/$@ $^

clean:
	rm -rf $(BINS)