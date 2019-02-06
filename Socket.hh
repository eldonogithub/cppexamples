#pragma once

#include <string>

class Socket
{
public:
  Socket(int fd, uint32_t events) : m_fd(fd), m_events(events) {}
  virtual ~Socket() {}

  int getFd()
  {
    return m_fd;
  }
  uint32_t getEvents()
  {
    return m_events;
  }
  bool readData()
  {
    return false;
  }
  bool writeData()
  {
    return false;
  }

protected:
  int m_fd;
  uint32_t m_events;

  char buffer[16];
  std::string message;
};
