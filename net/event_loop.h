#ifndef NET_EVENT_LOOP_H
#define NET_EVENT_LOOP_H_

#include "base/current_thread.h"
#include "base/noncopyable.h"

class EventLoop : noncopyable {
 public:
  EventLoop();
  ~EventLoop();

  void loop();
  void AssertInLoopThread() {
    if (!IsInLoopThread()) {
      AbortNotInLoopThread();
    }
  }

  bool IsInLoopThread() const {
    return threadid_ == currentthread::tid();
  }

  static EventLoop* GetEventLoopOfCurrentThread();

 private:
  void AbortNotInLoopThread();
  bool looping_;
  const pid_t threadid_;
};

#endif