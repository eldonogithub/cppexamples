#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <list>
#include <netdb.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Main.hh"

int main( int argc, char* argv[])
{
  // get the process id to use
  pid_t pid = getpid();

  std::map<int, std::string> buffer;

  if ( argc != 3 ) {
    std::cerr << "Usage: %s HOST PORT" << std::endl;
    return EXIT_FAILURE;
  }

  Main main(argc, argv);

  main.run();

  char buf[16];
  while ( 1 ) {
    int r = read(sfd, buf, sizeof(buf)-1);
    if ( r == -1 ) {
      if ( errno == EWOULDBLOCK ) {
        continue;
      }

      perror("read");
      return EXIT_FAILURE;
    }
    if ( r == 0 ) {
      std::cerr << "Socket closed" << std::endl;
      close(sfd);
      return EXIT_FAILURE;
    }
    buf[r] = 0;
    // find the input buffer for this socket?
    int fd = sfd;
    std::map<int, std::string>::iterator it = buffer.find(fd);

    // is there an input buffer for this socket?
    if ( it == buffer.end() ) {
      // No, so make one
      std::pair<std::map<int, std::string>::iterator, bool> ret;
      ret = buffer.insert(std::pair<int, std::string>(fd, std::string()));
      if ( ret.second == false ) {
        std::cerr << "Unable to insert into map" << std::endl;
        return EXIT_FAILURE;
      }
      it = ret.first;
    }

    it->second.append(buf);
    std::string::size_type pos = it->second.find("\r\n\r\n");
    // is there an end of request?
    if ( pos != std::string::npos ) {
      std::cerr << "Found Response: " << pos << std::endl;
      std::cerr << it->second.substr(0, pos) << std::endl;
      it->second.erase(0,pos+4);
    }
  }
}
