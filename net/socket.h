#ifndef NET_SOCKET_H_
#define NET_SECKET_H_

#include "base/noncopyable.h"

class InetAddress;

class Socket : noncopyable {
 public:
  explicit Socket(int sockfd) : sockfd_(sockfd) { }

  ~Socket();

  int fd() const {
    return sockfd_;
  }
  // bool GetTcpInfo(struct tcp_info*) const;
  // bool GetTcpInfoString(char* buf, int len) const;

  void BindAddress(const InetAddress& localaddr);
  void listen();

  int accept(InetAddress* peeraddr);

  void ShutdownWrite();

  // void SetTcpNoDelay(bool on);

  void SetReuseAddr(bool on);

  // void SetReusePort(bool on);

  // void SetKeepAlive(bool on);

 private:
  const int sockfd_;
};

#endif