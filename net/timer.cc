#include "net/timer.h"

#include "base/timestamp.h"

AtomicInt64 Timer::s_numcreated_;

void Timer::restart(Timestamp now) {
  if (repeat_) {
    expiration_ = AddTime(now, interval_);
  }
  else {
    expiration_ = Timestamp::invalid();
  }
}