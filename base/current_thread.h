#ifndef BASE_CURRENT_THREAD_H_
#define BASE_CURRENT_THREAD_H_

#include "base/types.h"

namespace currentthread {

extern __thread int t_cachedtid;
extern __thread char t_tidstring[32];
extern __thread int t_tidstringlength;
extern __thread const char* t_threadname;
void cachetid();

inline int tid() {
  if (__builtin_expect(t_cachedtid == 0, 0)) {
    cachetid();
  }
  return t_cachedtid;
}

inline const char* tidstring() {
  return t_tidstring;
} 

inline int tidstringlength() {
  return t_tidstringlength;
}

inline const char* threadname() {
  return t_threadname;
}

// bool IsMainThread();
// void SleepUsec(int64_t usec);
// string StackTrace(bool demangle);

}

#endif