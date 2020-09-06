#ifndef NET_TCP_SERVER_H_
#define NET_TCP_SERVER_H_

#include <map>

#include "net/tcp_connection.h"

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer : noncopyable {
 public:
  TcpServer(EventLoop* loop, const InetAddress& listenaddr);
  ~TcpServer();

  void SetThreadNum(int numthreads);

  void start();

  void SetConnectionCallback(const ConnectionCallback& cb) {
    connectioncallback_ = cb;
  }

  void SetMessageCallback(const MessageCallback& cb) {
    messagecallback_ = cb;
  }

  void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writecompletecallback_ = cb;
  }

 private:
  void NewConnection(int sockfd, const InetAddress& peeraddr);
  void RemoveConnection(const TcpConnectionPtr& conn);
  void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

  typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::unique_ptr<EventLoopThreadPool> threadpool_;
  ConnectionCallback connectioncallback_;
  MessageCallback messagecallback_;
  WriteCompleteCallback writecompletecallback_;
  bool started_;
  int nextconnid_;
  ConnectionMap connections_;
};

#endif