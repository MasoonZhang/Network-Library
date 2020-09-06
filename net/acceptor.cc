#include "net/acceptor.h"

#include "net/event_loop.h"
#include "net/inet_address.h"
#include "net/sockets_ops.h"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenaddr)
    : loop_(loop), acceptsocket_(sockets::CreateNonblockingOrDie()),
      acceptchannel_(loop, acceptsocket_.fd()), listenning_(false) {
  acceptsocket_.SetReuseAddr(true);
  acceptsocket_.BindAddress(listenaddr);
  acceptchannel_.SetReadCallback(std::bind(&Acceptor::HandleRead, this));
}

void Acceptor::listen() {
  loop_->AssertInLoopThread();
  listenning_ = true;
  acceptsocket_.listen();
  acceptchannel_.EnableReading();
}

void Acceptor::HandleRead() {
  loop_->AssertInLoopThread();
  InetAddress peeraddr(0);
  int connfd = acceptsocket_.accept(&peeraddr);
  if (connfd >= 0) {
    if (newconnectioncallback_) {
      newconnectioncallback_(connfd, peeraddr);
    } else {
      sockets::close(connfd);
    }
  }
}