#ifndef BASE_TIMESTAMP_H_
#define BASE_TIMESTAMP_H_

#include "base/types.h"

class Timestamp {
 public:
  Timestamp()
    : microsecondssinceepoch_(0) { }

  explicit Timestamp(int64_t microsecondssinceepocharg)
    : microsecondssinceepoch_(microsecondssinceepocharg) { }

  void swap(Timestamp& that) {
    std::swap(microsecondssinceepoch_, that.microsecondssinceepoch_);
  }

  string ToString() const;
  string ToFormattedString(bool showMicroseconds = true) const;

  bool valid() const {
    return microsecondssinceepoch_ > 0;
  }

  int64_t microsecondssinceepoch() const {
    return microsecondssinceepoch_;
  }
  time_t secondssinceepoch() const {
    return static_cast<time_t>(microsecondssinceepoch_ / kmicrosecondspersecond);
  }
  static Timestamp now();
  static Timestamp invalid() {
    return Timestamp();
  }

  static Timestamp FromUnixTime(time_t t) {
    return FromUnixTime(t, 0);
  }

  static Timestamp FromUnixTime(time_t t, int microseconds) {
    return Timestamp(static_cast<int64_t>(t) * kmicrosecondspersecond + microseconds);
  }

  static const int kmicrosecondspersecond = 1000 * 1000;

 private:
  int64_t microsecondssinceepoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microsecondssinceepoch() < rhs.microsecondssinceepoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microsecondssinceepoch() == rhs.microsecondssinceepoch();
}

inline double TimeDifference(Timestamp high, Timestamp low) {
  int64_t diff = high.microsecondssinceepoch() - low.microsecondssinceepoch();
  return static_cast<double>(diff) / Timestamp::kmicrosecondspersecond;
}

inline Timestamp AddTime(Timestamp timestamp, double seconds) {
  int64_t delta = static_cast<int64_t>(seconds * Timestamp::kmicrosecondspersecond);
  return Timestamp(timestamp.microsecondssinceepoch() + delta);
}

#endif