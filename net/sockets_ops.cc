#include "net/sockets_ops.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>

#include <iostream>

#include "base/types.h"

namespace {

typedef struct sockaddr SA;

#if VALGRIND || defined (NO_ACCEPT4)
void SetNonBlockAndCloseOnExec(int sockfd) {
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  (void)ret;
}
#endif

}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr) {
  return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in6* addr) {
  return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr) {
  return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

const struct sockaddr_in* sockets::sockaddr_in_cast(const struct sockaddr* addr) {
  return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

const struct sockaddr_in6* sockets::sockaddr_in6_cast(const struct sockaddr* addr) {
  return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
}

int sockets::CreateNonblockingOrDie() {
#if VALGRIND
  int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    std::cout << "sockets::CreateNonblockingOrDie" std::endl;
  }

  SetNonBlockAndCloseOnExec(sockfd);
#else
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) {
    std::cout << "sockets::CreateNonblockingOrDie" << std::endl;
  }
#endif
  return sockfd;
}

void sockets::BindOrDie(int sockfd, const struct sockaddr* addr) {
  int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    std::cout << "sockets::BindOrDie" << std::endl;
  }
}

void sockets::ListenOrDie(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    std::cout << "sockets::ListenOrDie" << std::endl;
  }
}

int sockets::accept(int sockfd, struct sockaddr_in6* addr) {
  socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND || defined (NO_ACCEPT4)
  int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
  SetNonBlockAndCloseOnExec(connfd);
#else
  int connfd = ::accept4(sockfd, sockaddr_cast(addr),
                         &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  if (connfd < 0) {
    int savederrno = errno;
    std::cout << "Socket::accept" << std::endl;
    switch (savederrno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:
      case EPERM:
      case EMFILE:
        errno = savederrno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        std::cout << "unexpected error of ::accept " << savederrno << std::endl;
        break;
      default:
        std::cout << "unknown error of ::accept " << savederrno << std::endl;
        break;
    }
  }
  return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr* addr) {
  return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

// ssize_t sockets::read(int sockfd, void *buf, size_t count)
// {
//   return ::read(sockfd, buf, count);
// }

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt)
{
  return ::readv(sockfd, iov, iovcnt);
}

// ssize_t sockets::write(int sockfd, const void *buf, size_t count)
// {
//   return ::write(sockfd, buf, count);
// }

void sockets::close(int sockfd) {
  if (::close(sockfd) < 0) {
    std::cout << "sockets::close" << std::endl;
  }
}

void sockets::ShutdownWrite(int sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    std::cout << "sockets::shutdownWrite" << std::endl;
  }
}

void sockets::ToIpPort(char* buf, size_t size,
                       const struct sockaddr* addr) {
  ToIp(buf,size, addr);
  size_t end = ::strlen(buf);
  const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
  uint16_t port = sockets::NetworkToHost16(addr4->sin_port);
  assert(size > end);
  snprintf(buf+end, size-end, ":%u", port);
}

void sockets::ToIp(char* buf, size_t size,
                   const struct sockaddr* addr) {
  if (addr->sa_family == AF_INET) {
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
  } else if (addr->sa_family == AF_INET6) {
    assert(size >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
  }
}

void sockets::FromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in* addr) {
  addr->sin_family = AF_INET;
  addr->sin_port = HostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    std::cout << "sockets::fromIpPort" << std::endl;
  }
}

void sockets::FromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in6* addr) {
  addr->sin6_family = AF_INET6;
  addr->sin6_port = HostToNetwork16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
    std::cout << "sockets::fromIpPort" << std::endl;
  }
}

int sockets::GetSocketError(int sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

struct sockaddr_in6 sockets::GetLocalAddr(int sockfd) {
  struct sockaddr_in6 localaddr;
  MemZero(&localaddr, sizeof localaddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
    std::cout << "sockets::GetLocalAddr" << std::endl;
  }
  return localaddr;
}

struct sockaddr_in6 sockets::GetPeerAddr(int sockfd) {
  struct sockaddr_in6 peeraddr;
  MemZero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
    std::cout << "sockets::getPeerAddr" << std::endl;
  }
  return peeraddr;
}

bool sockets::IsSelfConnect(int sockfd) {
  struct sockaddr_in6 localaddr = GetLocalAddr(sockfd);
  struct sockaddr_in6 peeraddr = GetPeerAddr(sockfd);
  if (localaddr.sin6_family == AF_INET) {
    const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
    const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port
        && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  } else if (localaddr.sin6_family == AF_INET6) {
    return localaddr.sin6_port == peeraddr.sin6_port
        && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
  } else {
    return false;
  }
}