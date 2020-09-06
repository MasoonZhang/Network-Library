#ifndef NET_TCP_CLIENT_H_
#define NET_TCP_CLIENT_H_

#include "base/mutex.h"
#include "net/tcp_connection.h"

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable {
 public:
  TcpClient(EventLoop* loop,
            const InetAddress& serverAddr);
  ~TcpClient();

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const {
    MutexLockGuard lock(mutex_);
    return connection_;
  }

  bool retry() const;
  void EnableRetry() {
    retry_ = true;
  }

  void SetConnectionCallback(const ConnectionCallback& cb) {
    connectioncallback_ = cb;
  }

  void SetMessageCallback(const MessageCallback& cb) {
    messagecallback_ = cb;
  }

  void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writecompletecallback_ = cb;
  }

 private:
  void NewConnection(int sockfd);
  void RemoveConnection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_;
  ConnectionCallback connectioncallback_;
  MessageCallback messagecallback_;
  WriteCompleteCallback writecompletecallback_;
  bool retry_;
  bool connect_;
  int nextconnid_;
  mutable MutexLock mutex_;
  TcpConnectionPtr connection_;
};

#endif