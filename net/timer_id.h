#ifndef NET_TIMER_ID_H
#define NET_TIMER_ID_H

#include <stdint.h>

class Timer;

class TimerId {
 public:
  TimerId(Timer* timer = NULL, int64_t seq = 0)
    : timer_(timer),
      sequence_(seq)
  { }
  
  friend class TimerQueue;

 private:
  Timer* timer_;
  int16_t sequence_;
};

#endif