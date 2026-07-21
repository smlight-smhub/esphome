#pragma once
#include "esphome/core/defines.h"

// Helper file to include all socket-related system headers (or use our own
// definitions where system ones don't exist)

#ifdef USE_SOCKET_IMPL_LWIP_TCP

#define LWIP_INTERNAL
#include "lwip/inet.h"
#include <cerrno>
#include <cstdint>
#include <sys/types.h>

/* Address families.  */
#define AF_UNSPEC 0
#define AF_INET 2
#define PF_INET AF_INET
#define PF_UNSPEC AF_UNSPEC

#define IPPROTO_IP 0
#define IPPROTO_TCP 6

#if LWIP_IPV6
#define AF_INET6 10
#define PF_INET6 AF_INET6

#define IPPROTO_IPV6 41
#define IPPROTO_ICMPV6 58
#endif

#define TCP_NODELAY 0x01

#define F_GETFL 3
#define F_SETFL 4

#ifdef O_NONBLOCK
#undef O_NONBLOCK
#endif
#define O_NONBLOCK 1

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3

#define SO_REUSEADDR 0x0004 /* Allow local address reuse */
#define SO_KEEPALIVE 0x0008 /* keep connections alive */
#define SO_BROADCAST 0x0020 /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */
#define SO_RCVTIMEO 0x1006  /* receive timeout */
#define SO_SNDTIMEO 0x1005  /* send timeout */

#define SOL_SOCKET 0xfff /* options for socket level */

using sa_family_t = uint8_t;
using in_port_t = uint16_t;

// NOLINTNEXTLINE(readability-identifier-naming)
struct sockaddr_in {
  uint8_t sin_len;
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
#define SIN_ZERO_LEN 8
  char sin_zero[SIN_ZERO_LEN];
};

#if LWIP_IPV6
// NOLINTNEXTLINE(readability-identifier-naming)
struct sockaddr_in6 {
  uint8_t sin6_len;          /* length of this structure    */
  sa_family_t sin6_family;   /* AF_INET6                    */
  in_port_t sin6_port;       /* Transport layer port #      */
  uint32_t sin6_flowinfo;    /* IPv6 flow information       */
  struct in6_addr sin6_addr; /* IPv6 address                */
  uint32_t sin6_scope_id;    /* Set of interfaces for scope */
};
#endif

// NOLINTNEXTLINE(readability-identifier-naming)
struct sockaddr {
  uint8_t sa_len;
  sa_family_t sa_family;
  char sa_data[14];
};

// NOLINTNEXTLINE(readability-identifier-naming)
struct sockaddr_storage {
  uint8_t s2_len;
  sa_family_t ss_family;
  char s2_data1[2];
  uint32_t s2_data2[3];
  uint32_t s2_data3[3];
};
using socklen_t = uint32_t;

// NOLINTNEXTLINE(readability-identifier-naming)
struct iovec {
  void *iov_base;
  size_t iov_len;
};

#if defined(USE_ESP8266) || defined(USE_RP2)
// arduino-esp8266 declares a global vars called INADDR_NONE/ANY which are invalid with the define
#ifdef INADDR_ANY
#undef INADDR_ANY
#endif
#ifdef INADDR_NONE
#undef INADDR_NONE
#endif

#define ESPHOME_INADDR_ANY ((uint32_t) 0x00000000UL)
#define ESPHOME_INADDR_NONE ((uint32_t) 0xFFFFFFFFUL)
#else  // !USE_ESP8266
#define ESPHOME_INADDR_ANY INADDR_ANY
#define ESPHOME_INADDR_NONE INADDR_NONE
#endif

#endif  // USE_SOCKET_IMPL_LWIP_TCP

#ifdef USE_SOCKET_IMPL_LWIP_SOCKETS

// standard lwIP's compatibility macros will interfere
// with Socket class function names - disable the macros
// and use real function names instead
#undef LWIP_COMPAT_SOCKETS
#define LWIP_COMPAT_SOCKETS 0

#include "lwip/sockets.h"
#include <sys/types.h>

#ifdef USE_ARDUINO
// arduino-esp32 declares a global var called INADDR_NONE which is replaced
// by the define
#ifdef INADDR_NONE
#undef INADDR_NONE
#endif
// not defined for ESP32
using socklen_t = uint32_t;

#define ESPHOME_INADDR_ANY ((uint32_t) 0x00000000UL)
#define ESPHOME_INADDR_NONE ((uint32_t) 0xFFFFFFFFUL)
#else  // !USE_ESP32
#define ESPHOME_INADDR_ANY INADDR_ANY
#define ESPHOME_INADDR_NONE INADDR_NONE
#endif

#endif  // USE_SOCKET_IMPL_LWIP_SOCKETS

#ifdef USE_SOCKET_IMPL_BSD_SOCKETS

#include <cstdint>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifndef USE_ZEPHYR
#include <sys/uio.h>
#endif
#include <unistd.h>

#ifdef USE_HOST
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#endif  // USE_HOST
#ifdef USE_ZEPHYR
#include <arpa/inet.h>
#include <netinet/in.h>
#endif  // USE_ZEPHYR

#ifdef USE_ARDUINO
// arduino-esp32 declares a global var called INADDR_NONE which is replaced
// by the define
#ifdef INADDR_NONE
#undef INADDR_NONE
#endif
// not defined for ESP32
using socklen_t = uint32_t;

#define ESPHOME_INADDR_ANY ((uint32_t) 0x00000000UL)
#define ESPHOME_INADDR_NONE ((uint32_t) 0xFFFFFFFFUL)
#else  // !USE_ESP32
#define ESPHOME_INADDR_ANY INADDR_ANY
#define ESPHOME_INADDR_NONE INADDR_NONE
#endif

#endif  // USE_SOCKET_IMPL_BSD_SOCKETS

#ifdef USE_SOCKET_IMPL_RPMSG_SOCKETS
#include <cstdint>
#include <sys/types.h>

#define AF_UNSPEC 0
#define AF_INET 2
#define PF_INET AF_INET

#define IPPROTO_IP 0
#define IPPROTO_TCP 6
#define TCP_NODELAY 0x01

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3

#define SO_REUSEADDR 0x0004
#define SO_KEEPALIVE 0x0008
#define SOL_SOCKET 0xfff

using sa_family_t = uint8_t;
using in_port_t = uint16_t;

struct in_addr {
  uint32_t s_addr;
};

struct sockaddr_in {
  uint8_t sin_len;
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
  char sin_zero[8];
};

struct sockaddr {
  uint8_t sa_len;
  sa_family_t sa_family;
  char sa_data[14];
};

struct sockaddr_storage {
  uint8_t s2_len;
  sa_family_t ss_family;
  char s2_data1[2];
  uint32_t s2_data2[3];
  uint32_t s2_data3[3];
};

using socklen_t = uint32_t;

struct iovec {
  void *iov_base;
  size_t iov_len;
};

using ip_addr_t = in_addr;
using ip4_addr_t = in_addr;

#include <cstdio>
inline int ipaddr_aton(const char *cp, struct in_addr *addr) {
    int a, b, c, d;
    if (sscanf(cp, "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
        addr->s_addr = a | (b << 8) | (c << 16) | (d << 24);
        return 1;
    }
    return 0;
}

#define ip_addr_set_zero(ip) ((ip)->s_addr = 0)
#define IP_ADDR4(ip, a, b, c, d) ((ip)->s_addr = (a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define ip_addr_copy(dest, src) ((dest).s_addr = (src).s_addr)
#define ipaddr_ntoa_r(ip, buf, buflen) \
    snprintf(buf, buflen, "%d.%d.%d.%d", \
             (int)(((ip)->s_addr) & 0xFF), \
             (int)(((ip)->s_addr >> 8) & 0xFF), \
             (int)(((ip)->s_addr >> 16) & 0xFF), \
             (int)(((ip)->s_addr >> 24) & 0xFF))
#define ip_addr_cmp(ip1, ip2) ((ip1)->s_addr == (ip2)->s_addr)
#define ip_addr_isany(ip) ((ip)->s_addr == 0)
#define IP_IS_V4(ip) (true)
#define IP_IS_V6(ip) (false)
#define ip_addr_ismulticast(ip) (((ip)->s_addr & 0xF0) == 0xE0)

#define ESPHOME_INADDR_ANY ((uint32_t) 0x00000000UL)
#define ESPHOME_INADDR_NONE ((uint32_t) 0xFFFFFFFFUL)

#define htons(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define htonl(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))

inline uint32_t inet_addr(const char *cp) {
    struct in_addr val;
    if (ipaddr_aton(cp, &val)) {
        return val.s_addr;
    }
    return ESPHOME_INADDR_NONE;
}

inline const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    if (af == AF_INET) {
        ipaddr_ntoa_r((const struct in_addr*)src, dst, size);
        return dst;
    }
    return nullptr;
}

#endif // USE_SOCKET_IMPL_RPMSG_SOCKETS


#if defined(USE_SOCKET_IMPL_LWIP_TCP) || defined(USE_SOCKET_IMPL_LWIP_SOCKETS) || defined(USE_SOCKET_IMPL_BSD_SOCKETS) || defined(USE_SOCKET_IMPL_RPMSG_SOCKETS)

namespace esphome::socket {

// Maximum length for formatted socket address string (IP address without port)
// IPv4: "255.255.255.255" = 15 chars + null = 16
// IPv6: full address = 45 chars + null = 46
#if USE_NETWORK_IPV6
static constexpr size_t SOCKADDR_STR_LEN = 46;  // INET6_ADDRSTRLEN
#else
static constexpr size_t SOCKADDR_STR_LEN = 16;  // INET_ADDRSTRLEN
#endif

}  // namespace esphome::socket

#endif
