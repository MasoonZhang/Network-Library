#ifndef NET_EVENT_LOOP_THREAD_H_
#define NET_EVENT_LOOP_THREAD_H_

#include "base/mutex.h"
#include "base/thread.h"

class EventLoop;

class EventLoopThread : noncopyable {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const string& name = string());
  ~EventLoopThread();
  EventLoop* StartLoop();

 private:
  void ThreadFunc();

  EventLoop* loop_;
  bool exiting_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
  ThreadInitCallback callback_;
};

#endif