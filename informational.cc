#include <unistd.h>
#include <sstream>

#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <map>
#include <sys/types.h>
#include <unistd.h>


class Connection
{
public:
  Connection(int fd) : m_socket(fd) {}

private:
  int m_socket;
};

int main( int argc, char* argv[])
{
  pid_t pid = getpid();

  std::map<int, std::string> buffer;

  if ( argc != 3 ) {
    std::cerr << "Usage: %s HOST PORT" << std::endl;
    return EXIT_FAILURE;
  }

  /*
   * Find all addresses for the host/port
   */

  struct addrinfo hints;
  struct addrinfo *result;

  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  int r;
  r = getaddrinfo(argv[1], argv[2], &hints, &result);
  if ( r != 0 ) {
    perror("getaddrinfo");
    return EXIT_FAILURE;
  }

  struct addrinfo *rp;
  int sfd;

  for ( rp = result; rp != NULL; rp = rp->ai_next ) {

    // socket() creates an endpoint for communication and returns a descriptor.
    sfd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if ( sfd == -1 ) {
      continue;
    }

    // int fcntl(int fd, int cmd, ... /* arg */ );
    int flags;
    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
      perror("fcntl");
      return EXIT_FAILURE;
    }

    // Set socket to nonblocking
    r = fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
    if (r == -1) {
      perror("fcntl");
      return EXIT_FAILURE;
    }

    r = connect(sfd, rp->ai_addr, rp->ai_addrlen);
    if ( r != -1 ) {
      break;
    }
    if ( r == -1 && errno == EINPROGRESS ) {
      break;
    }
    perror("connect");

    close(sfd);
  }

  if ( rp == NULL ) {
    std::cerr << "Could not connect" << std::endl;
    return EXIT_FAILURE;
  }
  socklen_t in_len;
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  in_len = sizeof(sockaddr);

  r = getnameinfo (rp->ai_addr, in_len,
                   hbuf, sizeof(hbuf),
                   sbuf, sizeof(sbuf),
                   NI_NUMERICHOST | NI_NUMERICSERV);
  if (r == 0) {

    printf("Connected to address "
           "(host=%s, port=%s)\n", hbuf, sbuf);

  }
  int requests = 0;
  for( int j = 0; j < 20; j++ ) {
    for( int i = 0; i < 10; i++ ) {
      requests++;
      std::stringstream s;
      s << "GET /test HTTP/1.1\r\n"
        << "Host:" << argv[1] << ":" << argv[2] << "\r\n"
        << "Accept: */*\r\n"
        << "Request-Id: " << pid << ":" << requests << "\r\n"
        << "Content-Length: 0\r\n"
        << "\r\n";
      std::string b = s.str();
      while(b.length() > 0 ) {
        // ssize_t write(int fd, const void *buf, size_t count);
        r = write(sfd, b.c_str(), b.length());

        if ( r == -1 ) {
          if ( errno == EWOULDBLOCK ) {
            continue;
          }
          perror("write");
          return EXIT_FAILURE;
        }
        std::cout << b.substr(0, r);

        b.erase(0,r);
      }
    }
    char buf[16];
    while ( 1 ) {
      r = read(sfd, buf, sizeof(buf)-1);
      if ( r == -1 ) {
        if ( errno == EWOULDBLOCK ) {
	  std::cout << "Going to block..." << std::endl;
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
        std::cout << "Found Response: " << pos << std::endl;
        std::cout << it->second.substr(0, pos+4) << std::endl;
        it->second.erase(0,pos+4);
      }
    }
  }
}
