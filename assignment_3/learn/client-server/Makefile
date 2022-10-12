CC = g++
CFLAGS = -g
BINS = server client

all: $(BINS)

%: %.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BINS)