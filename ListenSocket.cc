#include "Socket.hh"
#include <sys/epoll.h>

class ListenSocket : public Socket
{
public:
  ListenSocket(int fd) : Socket(fd, EPOLLIN | EPOLLET) {}
  virtual ~ListenSocket() {}
};

