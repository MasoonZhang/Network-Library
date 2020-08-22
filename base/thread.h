#ifndef BASE_THREAD_H_
#define BASE_THREAD_H_

#include <functional>

#include "base/count_down_latch.h"
#include "base/types.h"


class Thread : noncopyable {
 public:
  typedef std::function<void ()> ThreadFunc;
  explicit Thread(ThreadFunc, const string& name = string());
  ~Thread();

  void start();
  int join();

  bool started() const {
    return started_;
  }
  pid_t tid() const {
    return tid_;
  }
  const string& name() const {
    return name_;
  }

 private:
  void SetDefaultName();

  bool started_;
  bool joined_;
  pthread_t pthreadid_;
  pid_t tid_;
  ThreadFunc func_;
  string name_;
  CountDownLatch latch_;
};

#endif