#include "net/tcp_server.h"

#include <iostream>

#include "net/acceptor.h"
#include "net/event_loop.h"
#include "net/event_loop_thread_pool.h"
#include "net/sockets_ops.h"
#include "net/tcp_connection.h"

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenaddr)
  : loop_(loop),
    name_(listenaddr.ToIpPort()),
    acceptor_(new Acceptor(loop, listenaddr)),
    threadpool_(new EventLoopThreadPool(loop)),
    started_(false),
    nextconnid_(1) {
  acceptor_->SetNewConnectionCallback(
      std::bind(&TcpServer::NewConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
  loop_->AssertInLoopThread();
  std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing" << std::endl;

  // for (auto& item : connections_)
  // {
  //   TcpConnectionPtr conn(item.second);
  //   item.second.reset();
  //   conn->getLoop()->runInLoop(
  //     std::bind(&TcpConnection::connectDestroyed, conn));
  // }
}

void TcpServer::SetThreadNum(int numthreads) {
  assert(0 <= numthreads);
  threadpool_->SetThreadNum(numthreads);
}

void TcpServer::start() {
  if (!started_) {
    started_ = true;
    threadpool_->start();
  }

  if (!acceptor_->listenning()) {
    loop_->RunInLoop(
        std::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void TcpServer::NewConnection(int sockfd, const InetAddress& peeraddr) {
  loop_->AssertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", nextconnid_);
  ++nextconnid_;
  std::string connname = name_ + buf;
  std::cout << "TcpServer::NewConnection [" << name_ << "] - new connection ["
            << connname << "] from " << peeraddr.ToIpPort() << std::endl;
  InetAddress localaddr(sockets::GetLocalAddr(sockfd));
  EventLoop* ioloop = threadpool_->GetNextLoop();
  TcpConnectionPtr conn(new TcpConnection(ioloop, connname, sockfd, localaddr, peeraddr));
  connections_[connname] = conn;
  conn->SetConnectionCallback(connectioncallback_);
  conn->SetMessageCallback(messagecallback_);
  conn->SetWriteCompleteCallback(writecompletecallback_);
  conn->SetCloseCallback(std::bind(&TcpServer::RemoveConnection, this, _1));
  ioloop->RunInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn) {
  loop_->RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn) {
  loop_->AssertInLoopThread();
  std::cout << "TcpServer::RemoveConnectionInLoop [" << name_ << "] - connection " << conn->name() << std::endl;
  size_t n = connections_.erase(conn->name());
  assert(n == 1);
  (void)n;
  EventLoop* ioLoop = conn->GetLoop();
  ioLoop->QueueInLoop(
      std::bind(&TcpConnection::ConnectDestroyed, conn));
}