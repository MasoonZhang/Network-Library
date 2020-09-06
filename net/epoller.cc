#include "net/epoller.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>

#include "net/channel.h"
#include "net/event_loop.h"

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPoller::EPoller(EventLoop* loop)
  : ownerloop_(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kiniteventlistsize) {
  if (epollfd_ < 0) {
    std::cout << "EPoller::EPoller" << std::endl;
  }
}

EPoller::~EPoller() {
  ::close(epollfd_);
}

Timestamp EPoller::poll(int timeoutMs, ChannelList* activeChannels) {
  int numEvents = ::epoll_wait(epollfd_,
                               &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeoutMs);
  Timestamp now(Timestamp::now());
  if (numEvents > 0) {
    std::cout << numEvents << " events happended" << std::endl;
    FillActiveChannels(numEvents, activeChannels);
    if (implicit_cast<size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size()*2);
    }
  } else if (numEvents == 0) {
    std::cout << " nothing happended" << std::endl;
  }
  else {
    std::cout << "EPoller::poll()" << std::endl;
  }
  return now;
}

void EPoller::FillActiveChannels(int numEvents,
                                 ChannelList* activeChannels) const {
  assert(implicit_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i) {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EPoller::UpdateChannel(Channel* channel)
{
  AssertInLoopThread();
  std::cout << "fd = " << channel->fd() << " events = " << channel->events();
  const int index = channel->index();
  if (index == kNew || index == kDeleted) {
    int fd = channel->fd();
    if (index == kNew) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    }
    else {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }
    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  } else {
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->IsNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
     } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPoller::RemoveChannel(Channel* channel) {
  AssertInLoopThread();
  int fd = channel->fd();
  std::cout << "fd = " << fd << std::endl;
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->IsNoneEvent());
  int index = channel->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded) {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EPoller::AssertInLoopThread() {
    ownerloop_->AssertInLoopThread();
}

void EPoller::update(int operation, Channel* channel) {
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      std::cout << "epoll_ctl op=" << operation << " fd=" << fd << std::endl;
    } else {
      std::cout << "epoll_ctl op=" << operation << " fd=" << fd << std::endl;
    }
  }
}