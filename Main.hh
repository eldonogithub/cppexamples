#include "Socket.hh"
#include "ClientSocket.hh"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

#define MAXEVENTS 2048

class Main
{
public:
  Main(int argc, char* argv[])
  {
    m_argc = argc;
    m_argv = argv;

    /* Buffer where events are returned */
    m_events = ( epoll_event *)calloc (MAXEVENTS, sizeof(epoll_event));
  }

  void start()
  {
    // get the process id to use
    m_pid = getpid();

    m_host = m_argv[1];
    m_port = m_argv[2];

    int status = startConnect();
    if ( status != 0 ) {
      std::cerr << ": Failed to start" << std::endl;
      return;
    }
  }

  void run()
  {
    start();

    m_efd = epoll_create1 (0);
    if (m_efd == -1) {
      perror ("epoll_create");
      abort ();
    }

    for (std::map<int, Socket *>::iterator it=m_sockets.begin(); it != m_sockets.end(); ++it) {
      struct epoll_event event;
      int fd = it->first;
      event.data.fd = fd;
      event.events = it->second->getEvents();
      int s = epoll_ctl (m_efd, EPOLL_CTL_ADD, fd, &event);
      if (s == -1) {
        perror ("epoll_ctl");
        abort ();
      }
    }

    while(1) {
      int n;

      n = epoll_wait (m_efd, m_events, MAXEVENTS, -1);

      handleEvents(n);
    }
  }

  void handleEvents(int n)
  {
    for (int i = 0; i < n; i++) {
      int fd = m_events[i].data.fd;
      if ((m_events[i].events & EPOLLERR) ||
          (m_events[i].events & EPOLLHUP) ||
          (!(m_events[i].events & EPOLLIN))) {
        /* An error has occured on this fd, or the socket is not
           ready for reading (why were we notified then?) */
        fprintf (stderr, "epoll error\n");
        close (fd);
        continue;
	
      }
      std::map<int, Socket*>::iterator it = m_sockets.find(fd);
      if ( it != m_sockets.end() ) {
	it->second->readData();
      }
      else if (m_sfd == fd) {
        /* We have a notification on the listening socket, which
           means one or more incoming connections. */
        while (1) {
          struct sockaddr in_addr;
          socklen_t in_len;
          int infd;
          char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

          in_len = sizeof in_addr;
          infd = accept (m_sfd, &in_addr, &in_len);
          if (infd == -1) {
            if ((errno == EAGAIN) ||
                (errno == EWOULDBLOCK)) {
              /* We have processed all incoming
                 connections. */
              break;
            } else {
              perror ("accept");
              break;
            }
          }

          int s;
          s = getnameinfo (&in_addr, in_len,
                           hbuf, sizeof hbuf,
                           sbuf, sizeof sbuf,
                           NI_NUMERICHOST | NI_NUMERICSERV);
          if (s == 0) {
            printf("Accepted connection on descriptor %d "
                   "(host=%s, port=%s)\n", infd, hbuf, sbuf);
          }

          /* Make the incoming socket non-blocking and add it to the
             list of fds to monitor. */
          s = make_socket_non_blocking (infd);
          if (s == -1) {
            abort ();
          }

          struct epoll_event event;
          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          s = epoll_ctl (m_efd, EPOLL_CTL_ADD, infd, &event);
          if (s == -1) {
            perror ("epoll_ctl");
            abort ();
          }
        }
        continue;
      } else {
        /* data ready to read */
      }
    }
  }
  
private:
  void sendData(std::string b)
  {
    // Is there data already queued?
    if ( m_queue.empty() ) {

      // No, so send the data right away
      while(b.length() > 0 ) {
        // ssize_t write(int fd, const void *buf, size_t count);
        int r = write(m_sfd, b.c_str(), b.length());

        // Was there an error?
        if ( r == -1 ) {
          // If the write will block then we are done for now
          if ( errno == EWOULDBLOCK ) {
            break;
          }
           
          // there error was fatal
          perror("write");
          return;
        }
        std::cout << b.substr(0, r);

        b.erase(0,r);
      }
    }

    // is there still more to send?
    if ( ! b.empty() ) {
      // save it for later
      m_queue.push_back(b);
    }
  }

  void send()
  {
    socklen_t in_len;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    in_len = sizeof(sockaddr);

    int r = getnameinfo (m_rp->ai_addr, in_len,
                     hbuf, sizeof(hbuf),
                     sbuf, sizeof(sbuf),
                     NI_NUMERICHOST | NI_NUMERICSERV);
    if (r == 0) {

      printf("Connected to address "
             "(host=%s, port=%s)\n", hbuf, sbuf);

    }
    int requests = 0;
    for( int j = 0; j < 1; j++ ) {
      for( int i = 0; i < 1; i++ ) {
        requests++;
        std::stringstream s;
        s << "GET /test HTTP/1.1\r\n"
          << "Host:" << m_argv[1] << ":" << m_argv[2] << "\r\n"
          << "Accept: */*\r\n"
          << "Request-Id: " << m_pid << ":" << requests << "\r\n"
          << "Content-Length: 0\r\n"
          << "\r\n";
        std::string b = s.str();
        sendData(b);
      }
    }
  }
  int startConnect()
  {

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
    r = getaddrinfo(m_host, m_port, &hints, &result);
    if ( r != 0 ) {
      perror("getaddrinfo");
      return EXIT_FAILURE;
    }


    // find all IPs for the host to connect to
    for ( m_rp = result; m_rp != NULL; m_rp = m_rp->ai_next ) {

      // socket() creates an endpoint for communication and returns a descriptor.
      m_sfd = socket( m_rp->ai_family, m_rp->ai_socktype, m_rp->ai_protocol);

      if ( m_sfd == -1 ) {
        continue;
      }

      // int fcntl(int fd, int cmd, ... /* arg */ );
      int flags;
      flags = fcntl(m_sfd, F_GETFL, 0);
      if (flags == -1) {
        perror("fcntl");
        return EXIT_FAILURE;
      }

      r = fcntl(m_sfd, F_SETFL, flags | O_NONBLOCK);
      if (r == -1) {
        perror("fcntl");
        return EXIT_FAILURE;
      }

      r = connect(m_sfd, m_rp->ai_addr, m_rp->ai_addrlen);
      if ( r != -1 ) {
        break;
      }

      if ( r == -1 && errno == EINPROGRESS ) {
        break;
      }
      perror("connect");

      close(m_sfd);
    }

    if ( m_rp == NULL ) {
      std::cerr << "Could not connect" << std::endl;
      return EXIT_FAILURE;
    }

    m_sockets.insert(std::make_pair(m_sfd, new ClientSocket(m_sfd)));
  }

  int
  make_socket_non_blocking (int sfd)
  {
    int flags, s;

    flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1) {
      perror ("fcntl");
      return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sfd, F_SETFL, flags);
    if (s == -1) {
      perror ("fcntl");
      return -1;
    }

    return 0;
  }

private:
  int m_argc;
  char **m_argv;
  pid_t m_pid;

  struct addrinfo *m_rp;
  char *m_host;
  char *m_port;

  std::map<int, Socket *> m_sockets;
  struct epoll_event *m_events;

  std::vector<std::string> m_queue;
  int m_sfd; // server descriptor
  int m_efd; // event descriptor
};

