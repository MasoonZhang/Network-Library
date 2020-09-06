#include "net/timer_queue.h"

#include <assert.h>
#include <sys/timerfd.h>

#include <iostream>

#include "base/types.h"
#include "net/event_loop.h"
#include "net/timer.h"
#include "net/timer_id.h"

#include <unistd.h>

int CreateTimerfd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0) {
    std::cout << "Failed in timerfd_create" << std::endl;
  }
  return timerfd;
}

void ReadTimerfd(int timerfd, Timestamp now) {
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  std::cout << "TimerQueue::HandleRead() " << howmany << " at " << now.ToString() << std::endl;
  if (n != sizeof howmany) {
    std::cout << "TimerQueue::HandleRead() reads " << n << " bytes instead of 8" << std::endl;
  }
}

struct timespec HowMuchTimeFromNow(Timestamp when) {
  int64_t microseconds = when.microsecondssinceepoch()
                         - Timestamp::now().microsecondssinceepoch();
  if (microseconds < 100) {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(
      microseconds / Timestamp::kmicrosecondspersecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::kmicrosecondspersecond) * 1000);
  return ts;
}

void ResetTimerfd(int timerfd, Timestamp expiration) {
  struct itimerspec newvalue;
  struct itimerspec oldvalue;
  MemZero(&newvalue, sizeof newvalue);
  MemZero(&oldvalue, sizeof oldvalue);
  newvalue.it_value = HowMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newvalue, &oldvalue);
  if (ret) {
    std::cout << "timerfd_settime()" << std::endl;
  }
}


TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop), timerfd_(CreateTimerfd()),
      timerfdchannel_(loop, timerfd_), timers_() {
  timerfdchannel_.SetReadCallback(std::bind(&TimerQueue::HandleRead, this));
  timerfdchannel_.EnableReading();
}

TimerQueue::~TimerQueue() {
  timerfdchannel_.DisableAll();
  timerfdchannel_.remove();
  for (const Entry& timer : timers_) {
    delete timer.second;
  }
}

TimerId TimerQueue::AddTimer(const TimerCallback& cb, Timestamp when, double interval) {
  Timer* timer = new Timer(std::move(cb), when, interval);
  loop_->RunInLoop(std::bind(&TimerQueue::AddTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerid) {
  loop_->RunInLoop(
      std::bind(&TimerQueue::CancelInLoop, this, timerid));
}

void TimerQueue::AddTimerInLoop(Timer* timer) {
  loop_->AssertInLoopThread();
  bool earliestchanged = insert(timer);
  if (earliestchanged) {
    ResetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::CancelInLoop(TimerId timerid) {
  loop_->AssertInLoopThread();
  assert(timers_.size() == activetimers_.size());
  ActiveTimer timer(timerid.timer_, timerid.sequence_);
  ActiveTimerSet::iterator it = activetimers_.find(timer);
  if (it != activetimers_.end()) {
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1);
    (void)n;
    delete it->first;
    activetimers_.erase(it);
  } else if (callingexpiredtimers_) {
    cancelingtimers_.insert(timer);
  }
  assert(timers_.size() == activetimers_.size());
}

void TimerQueue::HandleRead() {
  loop_->AssertInLoopThread();
  Timestamp now(Timestamp::now());
  ReadTimerfd(timerfd_, now);
  std::vector<Entry> expired = GetExpired(now);

  callingexpiredtimers_ = true;
  cancelingtimers_.clear();
  for (std::vector<Entry>::iterator it = expired.begin();
      it != expired.end(); ++it) {
    it->second->run();
  }
  callingexpiredtimers_ = false;
  reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::GetExpired(Timestamp now) {
  assert(timers_.size() == activetimers_.size());
  std::vector<Entry> expired;
  Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  TimerList::iterator it = timers_.lower_bound(sentry);
  assert(it == timers_.end() || now < it->first);
  std::copy(timers_.begin(), it, back_inserter(expired));
  timers_.erase(timers_.begin(), it);

  for (const Entry& it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    size_t n = activetimers_.erase(timer);
    assert(n == 1);
    (void)n;
  }

  assert(timers_.size() == activetimers_.size());
  return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
  Timestamp nextexpire;
  for (std::vector<Entry>::const_iterator it = expired.begin();
      it != expired.end(); ++it) {
    ActiveTimer timer(it->second, it->second->sequence());
    if (it->second->repeat() && cancelingtimers_.find(timer) == cancelingtimers_.end()) {
      it->second->restart(now);
      insert(it->second);
    }
    else {
      delete it->second;
    }
  }
  if (!timers_.empty()) {
    nextexpire = timers_.begin()->second->expiration();
  }
  if (nextexpire.valid()) {
    ResetTimerfd(timerfd_, nextexpire);
  }
}

bool TimerQueue::insert(Timer* timer) {
  loop_->AssertInLoopThread();
  assert(timers_.size() == activetimers_.size());
  bool earliestchanged = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliestchanged = true;
  }
  {
  std::pair<TimerList::iterator, bool> result
    = timers_.insert(Entry(when, timer));
  assert(result.second); (void)result;
  }
  {
  std::pair<ActiveTimerSet::iterator, bool> result
    = activetimers_.insert(ActiveTimer(timer, timer->sequence()));
  assert(result.second); (void)result;
  }
  assert(timers_.size() == activetimers_.size());
  return earliestchanged;
}