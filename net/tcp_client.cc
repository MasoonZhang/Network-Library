#include "net/tcp_client.h"

#include <iostream>

#include "net/connector.h"
#include "net/event_loop.h"

namespace detail {

void RemoveConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
  loop->QueueInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
}

void RemoveConnector(const ConnectorPtr& connector)
{ }

}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr)
  : loop_(loop),
    connector_(new Connector(loop, serverAddr)),
    retry_(false),
    connect_(true),
    nextconnid_(1) {
  connector_->SetNewConnectionCallback(
      std::bind(&TcpClient::NewConnection, this, _1));
  std::cout << "TcpClient::TcpClient[" << this
           << "] - connector " << get_pointer(connector_) << std::endl;
}

TcpClient::~TcpClient() {
  std::cout << "TcpClient::~TcpClient[" << this
           << "] - connector " << get_pointer(connector_) << std::endl;
  TcpConnectionPtr conn;
  {
  MutexLockGuard lock(mutex_);
  conn = connection_;
  }
  if (conn) {
    CloseCallback cb = std::bind(&detail::RemoveConnection, loop_, _1);
    loop_->RunInLoop(
        std::bind(&TcpConnection::SetCloseCallback, conn, cb));
  } else {
    connector_->stop();
    loop_->RunAfter(1, std::bind(&detail::RemoveConnector, connector_));
  }
}

void TcpClient::connect() {
  std::cout << "TcpClient::connect[" << this << "] - connecting to "
           << connector_->serveraddress().ToIpPort() << std::endl;
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect() {
  connect_ = false;
  {
  MutexLockGuard lock(mutex_);
  if (connection_) {
    connection_->shutdown();
  }
  }
}

void TcpClient::stop() {
  connect_ = false;
  connector_->stop();
}

void TcpClient::NewConnection(int sockfd) {
  loop_->AssertInLoopThread();
  InetAddress peerAddr(sockets::GetPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.ToIpPort().c_str(), nextconnid_);
  ++nextconnid_;
  string connName = buf;

  InetAddress localAddr(sockets::GetLocalAddr(sockfd));
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));

  conn->SetConnectionCallback(connectioncallback_);
  conn->SetMessageCallback(messagecallback_);
  conn->SetWriteCompleteCallback(writecompletecallback_);
  conn->SetCloseCallback(
      std::bind(&TcpClient::RemoveConnection, this, _1));
  {
  MutexLockGuard lock(mutex_);
  connection_ = conn;
  }
  conn->ConnectEstablished();
}

void TcpClient::RemoveConnection(const TcpConnectionPtr& conn) {
  loop_->AssertInLoopThread();
  assert(loop_ == conn->GetLoop());

  {
  MutexLockGuard lock(mutex_);
  assert(connection_ == conn);
  connection_.reset();
  }

  loop_->QueueInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
  if (retry_ && connect_) {
    std::cout << "TcpClient::connect[" << this << "] - Reconnecting to "
             << connector_->serveraddress().ToIpPort() << std::endl;
    connector_->restart();
  }
}