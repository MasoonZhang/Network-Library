#ifndef NET_TIMER_H
#define NET_TIMER_H

#include "base/atomic.h"
#include "base/noncopyable.h"
#include "base/timestamp.h"
#include "net/callbacks.h"

class Timer : noncopyable {
 public:
  Timer(TimerCallback cb, Timestamp when, double interval)
    : callback_(std::move(cb)),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      sequence_(s_numcreated_.IncrementAndGet())
  { }

  void run() const {
    callback_();
  }

  Timestamp expiration() const {
    return expiration_;
  }
  bool repeat() const {
    return repeat_;
  }
  int64_t sequence() const {
    return sequence_;
  }

  void restart(Timestamp now);

 private:
  const TimerCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const bool repeat_;
  const int64_t sequence_;

  static AtomicInt64 s_numcreated_;
};

#endif