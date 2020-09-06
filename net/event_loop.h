#ifndef NET_EVENT_LOOP_H
#define NET_EVENT_LOOP_H_

#include <functional>
#include <memory>
#include <vector>

#include "base/current_thread.h"
#include "base/mutex.h"
#include "base/noncopyable.h"
#include "base/timestamp.h"
#include "net/callbacks.h"
#include "net/timer_id.h"

class Channel;
class EPoller;
class TimerQueue;

class EventLoop : noncopyable {
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();

  void loop();
  void quit();
  
  void RunInLoop(const Functor& cb);
  void QueueInLoop(const Functor& cb);

  TimerId RunAt(const Timestamp& time, const TimerCallback& cb);
  TimerId RunAfter(double delay, const TimerCallback& cb);
  TimerId RunEvery(double interval, const TimerCallback& cb);

  void cancel(TimerId timerid);
  void wakeup();
  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);

  void AssertInLoopThread() {
    if (!IsInLoopThread()) {
      AbortNotInLoopThread();
    }
  }

  bool IsInLoopThread() const {
    return threadid_ == currentthread::tid();
  }

  static EventLoop* GetEventLoopOfCurrentThread();

 private:
  void AbortNotInLoopThread();
  void HandleRead();
  void DoPendingFunctors();

  typedef std::vector<Channel*> ChannelList;

  bool looping_;
  bool quit_;
  bool callingpendingfunctors_;
  const pid_t threadid_;
  Timestamp pollreturntime_;
  std::unique_ptr<EPoller> poller_;
  std::unique_ptr<TimerQueue> timerqueue_;
  int wakeupfd_;
  std::unique_ptr<Channel> wakeupchannel_;
  ChannelList activechannels_;
  MutexLock mutex_;
  std::vector<Functor> pendingfunctors_;
};

#endif