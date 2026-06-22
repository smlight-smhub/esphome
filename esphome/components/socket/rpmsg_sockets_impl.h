/*
 * Copyright 2026 SMLIGHT
 */
#pragma once
#include "esphome/core/defines.h"

#ifdef USE_SOCKET_IMPL_RPMSG_SOCKETS

#include <memory>
#include <span>
#include <vector>
#include <string.h>

#include "esphome/core/helpers.h"
#include "headers.h"

namespace esphome::socket {

class RPMSGSocketImpl {
 public:
  RPMSGSocketImpl(int fd = 0);
  ~RPMSGSocketImpl();
  RPMSGSocketImpl(const RPMSGSocketImpl &) = delete;
  RPMSGSocketImpl &operator=(const RPMSGSocketImpl &) = delete;

  int connect(const struct sockaddr *addr, socklen_t addrlen) { return 0; }
  
  std::unique_ptr<RPMSGSocketImpl> accept(struct sockaddr *addr, socklen_t *addrlen);
  std::unique_ptr<RPMSGSocketImpl> accept_loop_monitored(struct sockaddr *addr, socklen_t *addrlen) {
      return accept(addr, addrlen);
  }

  int bind(const struct sockaddr *addr, socklen_t addrlen) { return 0; }
  int close();
  int shutdown(int how) { return 0; }

  int getpeername(struct sockaddr *addr, socklen_t *addrlen) { return 0; }
  int getsockname(struct sockaddr *addr, socklen_t *addrlen) { return 0; }

  size_t getpeername_to(std::span<char, SOCKADDR_STR_LEN> buf) {
      const char* name = "rpmsg_peer";
      size_t len = std::min((size_t)10, buf.size() - 1);
      memcpy(buf.data(), name, len);
      buf[len] = '\0';
      return len;
  }
  size_t getsockname_to(std::span<char, SOCKADDR_STR_LEN> buf) {
      return getpeername_to(buf);
  }

  int getsockopt(int level, int optname, void *optval, socklen_t *optlen) { return 0; }
  int setsockopt(int level, int optname, const void *optval, socklen_t optlen) { return 0; }
  int listen(int backlog) { return 0; }
  
  ssize_t read(void *buf, size_t len);
  ssize_t recvfrom(void *buf, size_t len, sockaddr *addr, socklen_t *addr_len) { return read(buf, len); }
  ssize_t readv(const struct iovec *iov, int iovcnt);
  
  ssize_t write(const void *buf, size_t len);
  ssize_t send(const void *buf, size_t len, int flags) { return write(buf, len); }
  ssize_t writev(const struct iovec *iov, int iovcnt);
  ssize_t sendto(const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
      return write(buf, len);
  }

  int setblocking(bool blocking) { return 0; }
  int loop() { return 0; }

  bool ready() const;

  int get_fd() const { return fd_; }

 protected:
  int fd_{-1};
};

}  // namespace esphome::socket

#endif  // USE_SOCKET_IMPL_RPMSG_SOCKETS
