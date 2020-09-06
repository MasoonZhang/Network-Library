#include "net/event_loop_thread_pool.h"

#include "net/event_loop.h"
#include "event_loop_thread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop)
  : baseloop_(baseloop),
    started_(false),
    numthreads_(0),
    next_(0) { }

EventLoopThreadPool::~EventLoopThreadPool() { }

void EventLoopThreadPool::start() {
  assert(!started_);
  baseloop_->AssertInLoopThread();

  started_ = true;

  for (int i = 0; i < numthreads_; ++i) {
    EventLoopThread* t = new EventLoopThread;
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->StartLoop());
  }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
  baseloop_->AssertInLoopThread();
  assert(started_);
  EventLoop* loop = baseloop_;

  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}