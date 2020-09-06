#include "net/inet_address.h"

#include "base/types.h"
#include "net/sockets_ops.h"

#include <netdb.h>
#include <netinet/in.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kinaddrany = INADDR_ANY;
static const in_addr_t kinaddrloopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t port, bool loopbackonly, bool ipv6) {
  static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
  static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
  if (ipv6) {
    MemZero(&addr6_, sizeof addr6_);
    addr6_.sin6_family = AF_INET6;
    in6_addr ip = loopbackonly ? in6addr_loopback : in6addr_any;
    addr6_.sin6_addr = ip;
    addr6_.sin6_port = sockets::HostToNetwork16(port);
  }
  else {
    MemZero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopbackonly ? kinaddrloopback : kinaddrany;
    addr_.sin_addr.s_addr = sockets::HostToNetwork32(ip);
    addr_.sin_port = sockets::HostToNetwork16(port);
  }
}

InetAddress::InetAddress(const string& ip, uint16_t port, bool ipv6) {
  if (ipv6) {
    MemZero(&addr6_, sizeof addr6_);
    sockets::FromIpPort(ip.c_str(), port, &addr6_);
  }
  else {
    MemZero(&addr_, sizeof addr_);
    sockets::FromIpPort(ip.c_str(), port, &addr_);
  }
}

string InetAddress::ToIpPort() const {
  char buf[64] = "";
  sockets::ToIpPort(buf, sizeof buf, GetSockAddr());
  return buf;
}

// string InetAddress::toIp() const
// {
//   char buf[64] = "";
//   sockets::toIp(buf, sizeof buf, getSockAddr());
//   return buf;
// }

// uint32_t InetAddress::ipNetEndian() const
// {
//   assert(family() == AF_INET);
//   return addr_.sin_addr.s_addr;
// }

// uint16_t InetAddress::toPort() const
// {
//   return sockets::networkToHost16(portNetEndian());
// }

// static __thread char t_resolveBuffer[64 * 1024];

// bool InetAddress::resolve(StringArg hostname, InetAddress* out)
// {
//   assert(out != NULL);
//   struct hostent hent;
//   struct hostent* he = NULL;
//   int herrno = 0;
//   memZero(&hent, sizeof(hent));

//   int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
//   if (ret == 0 && he != NULL)
//   {
//     assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
//     out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
//     return true;
//   }
//   else
//   {
//     if (ret)
//     {
//       LOG_SYSERR << "InetAddress::resolve";
//     }
//     return false;
//   }
// }

// void InetAddress::setScopeId(uint32_t scope_id)
// {
//   if (family() == AF_INET6)
//   {
//     addr6_.sin6_scope_id = scope_id;
//   }
// }