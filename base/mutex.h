#ifndef BASE_MUTEX_H_
#define BASE_MUTEX_H_

#include <assert.h>
#include <pthread.h>

#include "current_thread.h"
#include "base/noncopyable.h"

class MutexLock : noncopyable {
 public:
  MutexLock() : holder_(0) {
    pthread_mutex_init(&mutex_, nullptr);
  }
  ~MutexLock() {
    assert(holder_ == 0);
    pthread_mutex_destroy(&mutex_);
  }

  bool IsLockedByThisThread() {
    return holder_ == currentthread::tid();
  }
  void AssertLocked() {
    assert(IsLockedByThisThread());
  }

  void lock() {
    pthread_mutex_lock(&mutex_);
    holder_ = currentthread::tid();
  }
  void unlock() {
    holder_ = 0;
    pthread_mutex_unlock(&mutex_);
  }

  pthread_mutex_t* GetPthreadMutex() {
    return &mutex_;
  }

 private:
  pthread_mutex_t mutex_;
  pid_t holder_;
};

class MutexLockGuard : noncopyable {
 public:
  explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex) {
    mutex_.lock();
  }
  ~MutexLockGuard() {
    mutex_.unlock();
  }

 private:
  MutexLock& mutex_;
};

#define MutexLockGuard(x) static_assert(false, "missing mutex guard var name")

#endif