#ifndef NET_CONNECTOR_H_
#define NET_CONNECTOR_H_

#include <functional>
#include <memory>

#include "base/noncopyable.h"
#include "net/inet_address.h"
#include "net/timer_id.h"

class Channel;
class EventLoop;

class Connector : noncopyable {
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& serveraddr);
  ~Connector();

  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    newconnectioncallback_ = cb;
  }

  void start();
  void restart();
  void stop();
  const InetAddress& serveraddress() const {
    return serveraddr_;
  }

 private:
  enum States { kdisconnected, kconnecting, kconnected };
  static const int kmaxretrydelayms = 30 * 1000;
  static const int kinitretrydelayms = 500;

  void SetState(States s) {
    state_ = s;
  }
  void StartInLoop();
  void connect();
  void connecting(int sockfd);
  void HandleWrite();
  void HandleError();
  void retry(int sockfd);
  int RemoveAndResetChannel();
  void ResetChannel();

  EventLoop* loop_;
  InetAddress serveraddr_;
  bool connect_;
  States state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newconnectioncallback_;
  int retrydelayms_;
  TimerId timerid_;
};
typedef std::shared_ptr<Connector> ConnectorPtr;

#endif