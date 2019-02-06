# all: epoll_wait getifaddrs client

#LDFLAGS=-lcurl
# CFLAGS=-g
# CC=gcc
# CXX=g++ -g -std=c++98 -Wall -Wshadow
# all: pipeline epoll_wait getifaddrs client async_client input

.PHONY: all
all: pipeline epoll_wait getifaddrs client  input

LDFLAGS=--static -lcurl -L/home/eldon/local/lib -lboost_program_options
CFLAGS=-g
CC=gcc
CXX_OPTS=-g --coverage -Wall -std=c++11 -Wshadow -I/home/eldon/local/include
CXX=g++ $(CXX_OPTS)
CXX=g++ $(CXX_OPTS)

pipeline: pipeline.o
	$(CC) $^ $(LDFLAGS) -o $@

input: input.o 
	$(CXX) $^ $(LDFLAGS) -o $@

epoll_wait: epoll_wait.o 
	$(CXX) $^ $(LDFLAGS) -o $@

getifaddrs: getifaddrs.o 
	$(CXX) $^ -o $@

client: client.o 
	$(CXX) $^ -o $@

async_client: Socket.o ClientSocket.o ListenSocket.o Main.o async_client.o
	$(CXX) $^ -o $@

memory: memory.o
	$(CXX) $^ -o $@

auto_ptr: auto_ptr.o
	$(CXX) $^ -o $@

clean:
	$(RM) pipeline epoll_wait *.o
