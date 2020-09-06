#include "base/timestamp.h"

#include <sys/time.h>
#include <stdio.h>

#include <inttypes.h>

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp is same size as int64_t");

string Timestamp::ToString() const {
  char buf[32] = {0};
  int64_t seconds = microsecondssinceepoch_ / kmicrosecondspersecond;
  int64_t microseconds = microsecondssinceepoch_ % kmicrosecondspersecond;
  snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
  return buf;
}

string Timestamp::ToFormattedString(bool showmicroseconds) const {
  char buf[64] = {0};
  time_t seconds = static_cast<time_t>(microsecondssinceepoch_ / kmicrosecondspersecond);
  struct tm tm_time;
  gmtime_r(&seconds, &tm_time);

  if (showmicroseconds) {
    int microseconds = static_cast<int>(microsecondssinceepoch_ % kmicrosecondspersecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);
  } else {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

Timestamp Timestamp::now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  int64_t seconds = tv.tv_sec;
  return Timestamp(seconds * kmicrosecondspersecond + tv.tv_usec);
}
