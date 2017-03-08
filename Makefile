.PHONY: all
all: pipeline epoll_wait getifaddrs client

LDFLAGS=-lcurl
CFLAGS=-g
CC=gcc
CXX=g++ -g -std=c++14 -Wall -Wshadow

pipeline: pipeline.o
	$(CC) $^ $(LDFLAGS) -o $@

epoll_wait: epoll_wait.o 
	$(CXX) $^ $(LDFLAGS) -o $@

getifaddrs: getifaddrs.o 
	$(CXX) $^ -o $@

client: client.o 
	$(CXX) $^ -o $@

clean:
	$(RM) pipeline epoll_wait *.o
