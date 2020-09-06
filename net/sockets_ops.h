#ifndef NET_SOCKETS_OPS_H
#define NET_SOCKETS_OPS_H

#include <arpa/inet.h>
#include <endian.h>

namespace sockets {

inline uint64_t HostToNetwork64(uint64_t host64) {
  return htobe64(host64);
}

inline uint32_t HostToNetwork32(uint32_t host32) {
  return htobe32(host32);
}

inline uint16_t HostToNetwork16(uint16_t host16) {
  return htobe16(host16);
}

inline uint64_t NetworkToHost64(uint64_t net64) {
  return be64toh(net64);
}

inline uint32_t NetworkToHost32(uint32_t net32) {
  return be32toh(net32);
}

inline uint16_t NetworkToHost16(uint16_t net16) {
  return be16toh(net16);
}

int CreateNonblockingOrDie();

int  connect(int sockfd, const struct sockaddr* addr);
void BindOrDie(int sockfd, const struct sockaddr* addr);
void ListenOrDie(int sockfd);
int accept(int sockfd, struct sockaddr_in6* addr);
// ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
// ssize_t write(int sockfd, const void *buf, size_t count);
void close(int sockfd);
void ShutdownWrite(int sockfd);

void ToIpPort(char* buf, size_t size,
              const struct sockaddr* addr);
void ToIp(char* buf, size_t size,
          const struct sockaddr* addr);

void FromIpPort(const char* ip, uint16_t port,
                struct sockaddr_in* addr);
void FromIpPort(const char* ip, uint16_t port,
                struct sockaddr_in6* addr);

int GetSocketError(int sockfd);

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

struct sockaddr_in6 GetLocalAddr(int sockfd);
struct sockaddr_in6 GetPeerAddr(int sockfd);
bool IsSelfConnect(int sockfd);

}

#endif