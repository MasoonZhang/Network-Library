#ifndef EVENT_LOOP_THREAD_POOL_H_
#define EVENT_LOOP_THREAD_POOL_H_

#include <memory>
#include <vector>

#include "base/noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable {
 public:
  EventLoopThreadPool(EventLoop* baseloop);
  ~EventLoopThreadPool();
  void SetThreadNum(int numthreads) {
    numthreads_ = numthreads;
  }
  void start();
  EventLoop* GetNextLoop();

 private:
  EventLoop* baseloop_;
  bool started_;
  int numthreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};

#endif