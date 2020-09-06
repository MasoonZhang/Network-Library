#ifndef BASE_ATOMIC_H
#define BASE_ATOMIC_H

#include <stdint.h>

#include "base/noncopyable.h"

template<typename T>
class AtomicIntegerT : noncopyable {
 public:
  AtomicIntegerT() : value_(0) { }

  T get() {
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  T GetAndAdd(T x)  {
    return __sync_fetch_and_add(&value_, x);
  }

  T AddAndGet(T x) {
    return GetAndAdd(x) + x;
  }

  T IncrementAndGet() {
    return AddAndGet(1);
  }

  T DecrementAndGet() {
    return AddAndGet(-1);
  }

  void add(T x) {
    GetAndAdd(x);
  }

  void increment() {
    IncrementAndGet();
  }

  void decrement() {
    DecrementAndGet();
  }

  T GetAndSet(T newValue) {
    return __sync_lock_test_and_set(&value_, newValue);
  }

 private:
  volatile T value_;
};

typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;

#endif 