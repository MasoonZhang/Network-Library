#ifndef NET_INET_ADDRESS_H_
#define NET_INET_ADDRESS_H_

#include <netinet/in.h>
#include <string>
#include <vector>
#include <string>

namespace sockets {

const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);

}

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress {
 public:
  explicit InetAddress(uint16_t port = 0, bool loopbackonly = false, bool ipv6 = false);

  InetAddress(const std::string& ip, uint16_t port, bool ipv6 = false);

  explicit InetAddress(const struct sockaddr_in& addr)
    : addr_(addr)
  { }

  explicit InetAddress(const struct sockaddr_in6& addr)
    : addr6_(addr)
  { }

  // sa_family_t family() const { return addr_.sin_family; }
  // string toIp() const;
  std::string ToIpPort() const;
  // uint16_t toPort() const;

  const struct sockaddr* GetSockAddr() const {
    return sockets::sockaddr_cast(&addr6_);
  }
  void SetSockAddrInet6(const struct sockaddr_in6& addr6) {
    addr6_ = addr6;
  }

  // uint32_t ipNetEndian() const;
  // uint16_t portNetEndian() const { return addr_.sin_port; }

  // static bool resolve(const std::string& hostname, InetAddress* result);

  // void setScopeId(uint32_t scope_id);

 private:
  union {
    struct sockaddr_in addr_;
    struct sockaddr_in6 addr6_;
  };
};

#endif