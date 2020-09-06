#include "net/event_loop.h"

#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <syscall.h>
#include <sys/eventfd.h>

#include <iostream>

#include "net/channel.h"
#include "net/epoller.h"
#include "net/timer_id.h"
#include "net/timer_queue.h"

__thread EventLoop* t_loopinthisthread = 0;

const int kpolltimems = 10000;

int CreateEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    std::cout << "Failed in eventfd" << std::endl;
    abort();
  }
  return evtfd;
}

EventLoop* EventLoop::GetEventLoopOfCurrentThread() {
  return t_loopinthisthread;
}

class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

IgnoreSigPipe initObj;

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingpendingfunctors_(false),
      threadid_(currentthread::tid()),
      poller_(new EPoller(this)),
      timerqueue_(new TimerQueue(this)),
      wakeupfd_(CreateEventfd()),
      wakeupchannel_(new Channel(this, wakeupfd_)){
  std::cout << "EventLoop created " << this << " in thread" << threadid_ << std::endl;
  if (t_loopinthisthread) {
    std::cout << "Another EventLoop " << t_loopinthisthread
              << " exists in this thread " << threadid_ << std::endl;
  } else {
    t_loopinthisthread = this;
  }
  wakeupchannel_->SetReadCallback(
      std::bind(&EventLoop::HandleRead, this));
  wakeupchannel_->EnableReading();
}

EventLoop::~EventLoop() {
  assert(!looping_);
  ::close(wakeupfd_);
  t_loopinthisthread = nullptr;
}

void EventLoop::loop() {
  assert(!looping_);
  AssertInLoopThread();
  looping_ = true;
  quit_ = false;

  while (!quit_) {
    activechannels_.clear();
    pollreturntime_ = poller_->poll(kpolltimems, &activechannels_);
    for (ChannelList::iterator it = activechannels_.begin();
        it != activechannels_.end(); ++it) {
      (*it)->HandleEvent(pollreturntime_);
    }
    DoPendingFunctors();
  }

  std::cout << "EventLoop " << this << " stop looping" << std::endl;
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  if (!IsInLoopThread()) {
    wakeup();
  }
}

void EventLoop::RunInLoop(const Functor& cb) {
  if (IsInLoopThread()) {
    cb();
  } else {
    QueueInLoop(cb);
  }
}

void EventLoop::QueueInLoop(const Functor& cb) {
  {
  MutexLockGuard lock(mutex_);
  pendingfunctors_.push_back(cb);
  }
  if (!IsInLoopThread() || callingpendingfunctors_) {
    wakeup();
  }
}

TimerId EventLoop::RunAt(const Timestamp& time, const TimerCallback& cb) {
  return timerqueue_->AddTimer(cb, time, 0.0);
}

TimerId EventLoop::RunAfter(double delay, const TimerCallback& cb) {
  Timestamp time(AddTime(Timestamp::now(), delay));
  return RunAt(time, cb);
}
TimerId EventLoop::RunEvery(double interval, const TimerCallback& cb) {
  Timestamp time(AddTime(Timestamp::now(), interval));
  return timerqueue_->AddTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerid) {
  return timerqueue_->cancel(timerid);
}

void EventLoop::UpdateChannel(Channel* channel) {
  assert(channel->OwnerLoop() == this);
  AssertInLoopThread();
  poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel) {
  assert(channel->OwnerLoop() == this);
  AssertInLoopThread();
  poller_->RemoveChannel(channel);
}

void EventLoop::AbortNotInLoopThread() {
  std::cout << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadid_
            << ", current thread id = " <<  currentthread::tid() << std::endl;
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = ::write(wakeupfd_, &one, sizeof one);
  if (n != sizeof one) {
    std::cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << std::endl;
  }
}

void EventLoop::HandleRead()
{
  uint64_t one = 1;
  ssize_t n = ::read(wakeupfd_, &one, sizeof one);
  if (n != sizeof one) {
    std::cout << "EventLoop::handleRead() reads " << n << " bytes instead of 8" << std::endl;
  }
}

void EventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  callingpendingfunctors_ = true;

  {
  MutexLockGuard lock(mutex_);
  functors.swap(pendingfunctors_);
  }
  for (size_t i = 0; i < functors.size(); ++i) {
    functors[i]();
  }
  callingpendingfunctors_ = false;
}