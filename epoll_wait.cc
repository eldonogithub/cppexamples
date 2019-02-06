#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netdb.h>
//#include <regex>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int create_poll() {
  int efd = epoll_create1 (0);
  if (efd == -1) {
    perror ("epoll_create");
    abort ();
  }
  return efd;
}

void add_listen_socket(int efd, int sfd ) {
  struct epoll_event event;
  event.data.fd = sfd;
  event.events = EPOLLIN | EPOLLET;
  int s = epoll_ctl (efd, EPOLL_CTL_ADD, sfd, &event);
  if (s == -1) {
    perror ("epoll_ctl");
    abort ();
  }
}

static int
create_and_bind (char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  s = getaddrinfo (NULL, port, &hints, &result);
  if (s != 0) {
    fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
    return -1;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    socklen_t in_len;
    in_len = sizeof(sockaddr);
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    s = getnameinfo (rp->ai_addr, in_len,
                     hbuf, sizeof(hbuf),
                     sbuf, sizeof(sbuf),
                     NI_NUMERICHOST | NI_NUMERICSERV);
    if (s == 0) {
      printf("Listen address "
             "(host=%s, port=%s)\n", hbuf, sbuf);
    }
  }
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      continue;
    }

    int optval = 1;
    s = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (s != 0) {
      fprintf (stderr, "setsockopt: %s\n", gai_strerror (s));
      return -1;
    }
    s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
    if (s == 0) {
      socklen_t in_len;
      in_len = sizeof(sockaddr);
      char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
      s = getnameinfo (rp->ai_addr, in_len,
                       hbuf, sizeof(hbuf),
                       sbuf, sizeof(sbuf),
                       NI_NUMERICHOST | NI_NUMERICSERV);
      if (s == 0) {
        printf("Listening on descriptor %d "
               "(host=%s, port=%s)\n", sfd, hbuf, sbuf);
      } else {
        std::cerr << "Unable to determine listening addresss: " << gai_strerror(s) << std::endl;

      }
      /* We managed to bind successfully! */
      break;
    }

    close (sfd);
  }

  if (rp == NULL) {
    fprintf (stderr, "Could not bind\n");
    return -1;
  }

  freeaddrinfo (result);

  return sfd;
}


std::map<int, std::string> buffer;
std::map<int, std::string> response;

//std::regex regex("\\b(\r|\n)+\\b");
//std::regex regex("\r\n\r\n");
int message = 0;

static int
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

#define MAXEVENTS 2048

int
main (int argc, char *argv[])
{
  int sfd, s;
  int efd;
  struct epoll_event event;
  struct epoll_event *events;

  if (argc != 2) {
    fprintf (stderr, "Usage: %s [port]\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  sfd = create_and_bind (argv[1]);
  if (sfd == -1) {
    abort ();
  }

  s = make_socket_non_blocking (sfd);
  if (s == -1) {
    abort ();
  }

  s = listen (sfd, SOMAXCONN);
  if (s == -1) {
    perror ("listen");
    abort ();
  }

  efd = create_poll();

  // add listen socket 
  add_listen_socket(efd, sfd);

  /* Buffer where events are returned */
  events = ( epoll_event *)calloc (MAXEVENTS, sizeof event);

  /* The event loop */
  while (1) {
    int n, i;

    n = epoll_wait (efd, events, MAXEVENTS, -1);
    for (i = 0; i < n; i++) {
      if ((events[i].events & EPOLLERR) ||
          (events[i].events & EPOLLHUP) ||
          (!(events[i].events & EPOLLIN))) {
        /* An error has occured on this fd, or the socket is not
           ready for reading (why were we notified then?) */
        fprintf (stderr, "epoll error\n");
        close (events[i].data.fd);
        continue;
      }

      else if (sfd == events[i].data.fd) {
        /* We have a notification on the listening socket, which
           means one or more incoming connections. */
        while (1) {
          struct sockaddr in_addr;
          socklen_t in_len;
          int infd;
          char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

          in_len = sizeof in_addr;
          infd = accept (sfd, &in_addr, &in_len);
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

          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          s = epoll_ctl (efd, EPOLL_CTL_ADD, infd, &event);
          if (s == -1) {
            perror ("epoll_ctl");
            abort ();
          }
        }
        continue;
      } else {
        /* We have data on the fd waiting to be read. Read and
           display it. We must read whatever data is available
           completely, as we are running in edge-triggered mode
           and won't get a notification again for the same
           data. */
        int done = 0;

        while (1) {
          ssize_t count;
          char buf[512];

          count = read (events[i].data.fd, buf, sizeof(buf) - 1);
          if (count == -1) {
            /* If errno == EAGAIN, that means we have read all
               data. So go back to the main loop. */
            if (errno != EAGAIN) {
              perror ("read");
              done = 1;
            }
            break;
          } else if (count == 0) {
            /* End of file. The remote has closed the
               connection. */
            done = 1;
            break;
          }

          /* Write the buffer to standard output */

          // terminate the input buffer
          buf[count] = 0;

          // find the input buffer for this socket?
          int fd = events[i].data.fd;
          std::map<int, std::string>::iterator it = buffer.find(fd);

          // is there an input buffer for this socket?
          if ( it == buffer.end() ) {
            // No, so make one
            std::pair<std::map<int, std::string>::iterator, bool> ret;
            ret = buffer.insert(std::pair<int, std::string>(fd, std::string()));
            if ( ret.second == false ) {
              std::cerr << "Unable to insert into map" << std::endl;
              done = 1;
              break;
            }
            it = ret.first;
          }

          it->second.append(buf);
          std::string::size_type pos = it->second.find("\r\n\r\n");
          // is there an end of request?
          if ( pos != std::string::npos ) {
            //std::cerr << "Found Message: Length: " << pos << " FD: " << fd << std::endl;
            std::string msg = it->second.substr(0, pos);

            // skip the method line
            std::size_t mpos = msg.find("\r\n");
            std::size_t start = mpos+2;

            // find all headers
            std::map<std::string, std::string> headers;
            for(std::size_t hpos = msg.find("\r\n",start); hpos != std::string::npos; hpos = msg.find("\r\n", start) ) {
              std::string header = msg.substr(start, hpos-start);
              std::size_t cpos = header.find(":");

              std::string key = header.substr(0, cpos);
              std::string value = header.substr(cpos+1, hpos);
              headers.insert(std::pair<std::string,std::string>(key ,value ));
              //std::cerr << "#Header: " << key << ": [" << value << "]" << std::endl;
              start = hpos+2;
            }
            // std::cerr << it->second.substr(0, pos) << std::endl;
            message++;

            std::map<std::string, std::string>::iterator rit = headers.find("Request-Id");
            std::stringstream id;
            if ( rit != headers.end()) {
              id << "Response-Id: " << rit->second << ":" << fd;
            }
            std::map<std::string, std::string>::iterator eit = headers.find("Expect");
            std::stringstream httpresp;
            if ( eit != headers.end()) {
              id << "Expect: " << eit->second << ":" << fd;
            httpresp << "HTTP/1.1 100 Continue\r\n"
                     << "\r\n";
            }
            httpresp << "HTTP/1.1 200 OK\r\n"
                     << "Server: " << message << " FD: " << fd << "\r\n"
                     << id.str() << "\r\n"
                     << "Content-Length: 0\r\n"
                     << "\r\n";
            // Found request
            s = write (fd, httpresp.str().c_str(), httpresp.str().length());
            //std::cerr << "Response: " << std::endl;
            //std::cerr << httpresp.str() << std::endl;

            it->second.erase(0,pos+4);
          }
//                  Searches for all matches of regex, prints prefix, match, suffix
//                  std::smatch match;
//                  if ( std::regex_search(it->second, match, regex) ) {
//                    std::cerr << "Match found for 'buffer'\n";
//                    std::cerr << "Prefix: '" << match.prefix() << "'\n";
//                    for (size_t i = 0; i < match.size(); ++i) {
//                      std::cerr << i << ": match: " << match[i].length() << '\n';
//                    }
//                    std::cerr << "Suffix: '" << match.suffix() << "\'\n\n";
//                  }
        }

        if (done) {
          printf ("Closed connection on descriptor %d\n",
                  events[i].data.fd);

          /* Closing the descriptor will make epoll remove it
             from the set of descriptors which are monitored. */
          close (events[i].data.fd);
        }
      }
    }
  }

  free (events);

  close (sfd);

  return EXIT_SUCCESS;
}

