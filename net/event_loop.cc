#include "net/event_loop.h"

#include <poll.h>

#include <assert.h>
#include <iostream>
#include <syscall.h>

__thread EventLoop* t_loopinthisthread = 0;

EventLoop* EventLoop::GetEventLoopOfCurrentThread() {
  return t_loopinthisthread;
}

EventLoop::EventLoop()
    : looping_(false), threadid_(currentthread::tid()) {
  std::cout << "EventLoop created " << this << " in thread" << threadid_ << std::endl;
  if (t_loopinthisthread) {
    std::cout << "Another EventLoop " << t_loopinthisthread
              << " exists in this thread " << threadid_ << std::endl;
  } else {
    t_loopinthisthread = this;
  }
}

EventLoop::~EventLoop() {
  assert(!looping_);
  t_loopinthisthread = nullptr;
}

void EventLoop::loop() {
  assert(!looping_);
  AssertInLoopThread();
  looping_ = true;

  ::poll(nullptr, 0, 5 * 1000);

  std::cout << "EventLoop " << this << " stop looping" << std::endl;
  looping_ = false;
}

void EventLoop::AbortNotInLoopThread() {
  std::cout << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadid_
            << ", current thread id = " <<  currentthread::tid() << std::endl;
}