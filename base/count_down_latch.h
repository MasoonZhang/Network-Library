#ifndef BASE_COUNT_DOWN_LATCH_H_
#define BASE_COUNT_DOWN_LATCH_H_

#include "base/condition.h"
#include "base/mutex.h"
#include "base/noncopyable.h"

class CountDownLatch : noncopyable {
 public:
  CountDownLatch(int count);

  void wait();
  
  void CountDown();
  
  int GetCount() const;

 private:
  mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};

#endif