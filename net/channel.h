#ifndef NET_CHANNEL_H_
#define NET_CHANNEL_H_

#include <functional>
#include <memory>

#include "base/noncopyable.h"
#include "base/timestamp.h"

class EventLoop;

class Channel : noncopyable {
 public:
  typedef std::function<void()> EventCallback;
  typedef std::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void HandleEvent(Timestamp receivetime);
  void SetReadCallback(const ReadEventCallback& cb) {
    readcallback_ = cb;
  }
  void SetWriteCallback(const EventCallback& cb) {
    writecallback_ = cb;
  }
  void SetCloseCallback(const EventCallback& cb) {
    closecallback_ = cb;
  }
  void SetErrorCallback(const EventCallback& cb) {
    errorcallback_ = cb;
  }
  

  void tie(const std::shared_ptr<void>&);

  int fd() const {
    return fd_;
  }
  int events() {
    return events_;
  }
  void set_revents(int revt) {
    revents_ = revt;
  }
  bool IsNoneEvent() const {
    return events_ == knoneevent;
  }

  void EnableReading() {
    events_ |= kreadevent;
    update();
  }
  void EnableWriting() {
    events_ |= kwriteevent;
    update();
  }
  void DisableWriting() {
    events_ &= ~kwriteevent;
    update();
  }
  void DisableAll() {
    events_ = knoneevent;
    update();
  }
  bool IsWriting() {
    return events_ & kwriteevent;
  }
  
  int index() {
    return index_;
  }
  void set_index(int idx) {
    index_ = idx;
  }

  EventLoop* OwnerLoop() {
    return loop_;
  }
  void remove();

 private:
  void update();

  static const int knoneevent;
  static const int kreadevent;
  static const int kwriteevent;

  EventLoop* loop_;
  const int fd_;
  int events_;
  int revents_;
  int index_;

  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventhandling_;
  ReadEventCallback readcallback_;
  EventCallback writecallback_;
  EventCallback closecallback_;
  EventCallback errorcallback_;
};

#endif