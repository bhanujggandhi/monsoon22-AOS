CC = g++
CFLAGS = -lcrypto
BINS = client

all: tracker $(BINS)

tracker: tracker.cpp
	$(CC) -o $@ $^

%: %.cpp
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm -rf tracker
	rm -rf client