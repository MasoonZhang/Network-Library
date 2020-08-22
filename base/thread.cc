#include "base/thread.h"

#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t gettid() {
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

void AfterFork()
{
  currentthread::t_cachedtid = 0;
  currentthread::t_threadname = "main";
  currentthread::tid();
}

class ThreadNameInitializer {
 public:
  ThreadNameInitializer() {
    currentthread::t_threadname = "main";
    currentthread::tid();
    pthread_atfork(nullptr, nullptr, &AfterFork);
  }
};

ThreadNameInitializer init;

struct ThreadData {
  typedef Thread::ThreadFunc ThreadFunc;
  ThreadFunc func_;
  string name_;
  pid_t* tid_;
  CountDownLatch* latch_;

  ThreadData(ThreadFunc func, const string& name, pid_t* tid, CountDownLatch* latch)
    : func_(func), name_(name), tid_(tid), latch_(latch)
  { }

  void RunInThread() {
    *tid_ = currentthread::tid();
    tid_ = nullptr;
    latch_->CountDown();
    latch_ = nullptr;
    currentthread::t_threadname = name_.empty() ? "Thread" : name_.c_str();
    ::prctl(PR_SET_NAME, currentthread::t_threadname);
    
    func_();
    currentthread::t_threadname = "finished";
  }
};

void* StartThread(void* obj) {
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->RunInThread();
  delete data;
  return nullptr;
}

void currentthread::cachetid() {
  if (t_cachedtid == 0) {
    t_cachedtid = gettid();
    t_tidstringlength = snprintf(t_tidstring, sizeof t_tidstring, "%5d ", t_cachedtid);
  }
}

// bool currentthread::IsMainThread() {
//   return tid() == ::gettid();
// }

Thread::Thread(ThreadFunc func, const string& n)
    : started_(false), joined_(false), pthreadid_(0),
      tid_(0), func_(std::move(func)), name_(n), latch_(1) {
  SetDefaultName();
}

Thread::~Thread() {
  if (started_ && !joined_) {
    pthread_detach(pthreadid_);
  }
}

void Thread::SetDefaultName() {
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread");
    name_ = buf;
  }
}

void Thread::start() {
  assert(!started_);
  started_ = true;
  ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthreadid_, nullptr, &StartThread, data)) {
    started_ = false;
    delete data;
  } else {
    latch_.wait();
    assert(tid_ > 0);
  }
}

int Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthreadid_, nullptr);
}