#ifndef NET_TCP_CONNECTION_H_
#define NET_TCP_CONNECTION_H_

#include <string>

#include "base/noncopyable.h"
#include "base/types.h"
#include "net/buffer.h"
#include "net/callbacks.h"
#include "net/inet_address.h"

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
 public:
  TcpConnection(EventLoop* loop, const string& namearg, int sockfd,
                const InetAddress& localAddr, const InetAddress& peerAddr);

  ~TcpConnection();

  EventLoop* GetLoop() const {
    return loop_;
  }
  const string& name() const {
    return name_;
  }
  // const InetAddress& localaddress() const {
  //   return localaddr_;
  // }
  const InetAddress& peeraddress() const {
    return peeraddr_;
  }
  bool connected() const {
    return state_ == kconnected;
  }

  void send(const std::string& message);
  void shutdown();
  // void SetTcpNoDelay(bool on);

  void SetConnectionCallback(const ConnectionCallback& cb) {
    connectioncallback_ = cb;
  }

  void SetMessageCallback(const MessageCallback& cb) {
    messagecallback_ = cb;
  }

  void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writecompletecallback_ = cb;
  }

  void SetCloseCallback(const CloseCallback& cb) {
    closecallback_ = cb;
  }

  void ConnectEstablished();
  void ConnectDestroyed();

 private:
  enum StateE { kconnecting, kconnected, kdisconnected, kdisconnecting, };

  void SetState(StateE s) {
    state_ = s;
  }
  void HandleRead(Timestamp receivetime);
  void HandleWrite();
  void HandleClose();
  void HandleError();
  void SendInLoop(const std::string& message);
  void ShutdownInLoop();

  EventLoop* loop_;
  std::string name_;
  StateE state_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  InetAddress localaddr_;
  InetAddress peeraddr_;
  ConnectionCallback connectioncallback_;
  MessageCallback messagecallback_;
  WriteCompleteCallback writecompletecallback_;
  CloseCallback closecallback_;
  Buffer inputbuffer_;
  Buffer outputbuffer_;
};

#endif