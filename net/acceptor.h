#ifndef NET_ACCEPTOR_H_
#define NET_ACCEPTOR_H_

#include "net/channel.h"
#include "net/socket.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
 public:
  typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenaddr);
  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    newconnectioncallback_ = cb;
  }

  bool listenning() const {
    return listenning_;
  }
  void listen();

 private:
  void HandleRead();

  EventLoop* loop_;
  Socket acceptsocket_;
  Channel acceptchannel_;
  NewConnectionCallback newconnectioncallback_;
  bool listenning_;
};

#endif