#ifndef BASE_CONDITION_H_
#define BASE_CONDITION_H_

#include <pthread.h>

#include "base/mutex.h"
#include "base/noncopyable.h"

class Condition : noncopyable {
 public:
  explicit Condition(MutexLock& mutex) : mutex_(mutex) {
    pthread_cond_init(&pcond_, nullptr);
  }
  ~Condition() {
    pthread_cond_destroy(&pcond_);
  }

  void wait() {
    pthread_cond_wait(&pcond_, mutex_.GetPthreadMutex());
  }
  void notify() {
    pthread_cond_signal(&pcond_);
  }
  void notifyAll() {
    pthread_cond_broadcast(&pcond_);
  }

 private:
  MutexLock& mutex_;
  pthread_cond_t pcond_;
};

#endif