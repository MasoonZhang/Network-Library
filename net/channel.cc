#include "net/channel.h"

#include <assert.h>
#include <poll.h>

#include <iostream>
#include <memory>

#include "event_loop.h"

const int Channel::knoneevent = 0;
const int Channel::kreadevent = POLLIN | POLLPRI;
const int Channel::kwriteevent = POLLOUT;

Channel::Channel(EventLoop* loop, int fdarg)
    : loop_(loop), fd_(fdarg), events_(0), revents_(0),
     index_(-1), tied_(false), eventhandling_(false) { }

Channel::~Channel() {
  assert(!eventhandling_);
}

void Channel::tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::update() {
  loop_->UpdateChannel(this);
}

void Channel::remove() {
  assert(IsNoneEvent());
  loop_->RemoveChannel(this);
}

void Channel::HandleEvent(Timestamp receivetime) {
  eventhandling_ = true;
  if (events_ & POLLNVAL) {
    std::cout << "Channel::handle_event() POLLNVAL " << std::endl;
  }
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    std::cout << "Channel::handle_event() POLLHUP" << std::endl;
    if (closecallback_) {
      closecallback_();
    }
  }
  if (events_ & (POLLERR | POLLNVAL)) {
    if (errorcallback_) {
      errorcallback_();
    }
  }
  if (events_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (readcallback_) {
      readcallback_(receivetime);
    }
  }
  if (events_ & POLLOUT) {
    if (writecallback_) {
      writecallback_();
    }
  }
  eventhandling_ = false;
}