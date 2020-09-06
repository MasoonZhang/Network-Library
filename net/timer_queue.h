#ifndef NET_TIMER_QUEUE_H_
#define NET_TIMER_QUEUE_H

#include <set>
#include <vector>

#include "net/callbacks.h"
#include "net/channel.h"

class EventLoop;
class TimerId;
class Timer;

class TimerQueue : noncopyable {
 public:
  TimerQueue(EventLoop* loop);
  ~TimerQueue();

  TimerId AddTimer(const TimerCallback& cb, Timestamp when, double interval);

  void cancel(TimerId timerid);

 private:
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void AddTimerInLoop(Timer* timer);
  void CancelInLoop(TimerId timerid);
  void HandleRead();
  std::vector<Entry> GetExpired(Timestamp now);
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;
  const int timerfd_;
  Channel timerfdchannel_;
  TimerList timers_;

  bool callingexpiredtimers_;
  ActiveTimerSet activetimers_;
  ActiveTimerSet cancelingtimers_;
};

#endif