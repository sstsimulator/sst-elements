#ifndef __QSIM_NET_H
#define __QSIM_NET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#ifdef DEBUG
#undef DEBUG
#include <iostream>
#include <stdio.h>

static void dump(const char *s, size_t n) {
  while (n > 0) {
    unsigned i, dif = (n > 16)?16:n;
    for (i = 0; i < dif; i++) std::cout << std::hex << std::setw(2) 
                                        << std::setfill('0') 
                                        << (s[i]&0xff) << ' ';
    for (i = dif; i < 16; i++) std::cout << "   ";
    for (i = 0; i < dif; i++) std::cout << (isprint(s[i])?s[i]:'~');
    std::cout << '\n';
    n -= dif;
    s += dif;
  }
}

#endif

namespace QsimNet {

  static bool senddata(int socket_fd, const char *message, size_t n) {
     while (n > 0) {
#ifdef DEBUG
       std::cout << "Send to " << socket_fd << ":\n";
       dump(message, n);
#endif
       int rval = send(socket_fd, message, n, 0);
       if (rval == -1) {
         // TODO: Handle remote disconnect, etc. gracefully.
         return false;
       } else {
         message += rval;
         n -= rval;
       }
     }
     return true;
  }

  static bool recvdata(int socket_fd, char *buf, size_t n) {
    while (n > 0) {
      int rval = recv(socket_fd, buf, n, 0);
#ifdef DEBUG
      std::cout << "Recv on " << socket_fd << ":\n";
      dump(buf, n);
#endif
      if (rval == -1) {
        // TODO: Handle remote disconnect, etc. gracefully.
        return false;
      } else {
        buf += rval;
        n -= rval;
      }
    }
    return true;
  }

  struct SockBinStream {
    int fd;
    SockBinStream(int fd) : fd(fd) {}
  };

  struct SockBinStreamError {};

  template <typename T> bool sockBinGet(int socket_fd, T& d) {
    return recvdata(socket_fd, (char *)&d, sizeof(d));
  }

  template <typename T> bool sockBinPut(int socket_fd, const T& d) {
    return senddata(socket_fd, (char *)&d, sizeof(d));
  }

  template <typename T> SockBinStream &operator>>(SockBinStream &g, T& d) {
    if (!sockBinGet(g.fd, d)) throw SockBinStreamError();
    return g;
  }

  template <typename T> SockBinStream &operator<<(SockBinStream &g, const T& d) 
  {
    if (!sockBinPut(g.fd, d)) throw SockBinStreamError();
    return g;
  }
}

static inline int create_listen_socket(const char *port, unsigned backlog) {
  struct addrinfo hints, *r;
  int listenfd;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, port, &hints, &r);
  listenfd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
  if (bind(listenfd, r->ai_addr, r->ai_addrlen) ||
      listen(listenfd, backlog))
  {
    std::cout << "Could not listen on port " << port 
              << ". Probably still in TIME_WAIT state.\n";
    exit(1);
  }

  return listenfd;
}

static inline int client_socket(const char *servaddr, const char *port) {
  struct addrinfo hints, *serv;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int rval = getaddrinfo(servaddr, port, &hints, &serv);
  if (rval) {
    std::cout << "Could not connect to server.\n";
    exit(1);
  }

  int socket_fd;
  for (struct addrinfo *p = serv; p; p = p->ai_next) { 
    socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (socket_fd == -1) continue;

    rval = connect(socket_fd, p->ai_addr, p->ai_addrlen);
    if (rval) { close(socket_fd); socket_fd = -1; continue; }
  }

  if (socket_fd == -1) {
    std::cout << "Could not connect to server.\n";
    exit(1);
  }

  return socket_fd;
}

static inline int next_connection(int listenfd) {
  struct sockaddr_storage remote_ad;
  socklen_t addr_len = sizeof(remote_ad);
  int fd = accept(listenfd, (struct sockaddr *)&remote_ad, &addr_len);
}

#endif
