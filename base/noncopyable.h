#ifndef BASE_NONCOPYABLE_H_
#define BASE_NONCOPYABLE_H_

class noncopyable {
public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;
protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

#endif