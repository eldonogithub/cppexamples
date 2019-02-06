#include "Socket.hh"
#include <sys/epoll.h>

class ClientSocket : public Socket
{
public:
  ClientSocket(int fd) : Socket(fd, EPOLLOUT | EPOLLIN | EPOLLET) {}

  virtual ~ClientSocket() {}
};

