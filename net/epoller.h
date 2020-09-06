#ifndef MUDUO_NET_EPOLLER_H
#define MUDUO_NET_EPOLLER_H

#include <map>
#include <vector>

#include "base/noncopyable.h"
#include "base/timestamp.h"

struct epoll_event;

class Channel;
class EventLoop;

class EPoller : noncopyable {
 public:
  typedef std::vector<Channel*> ChannelList;

  EPoller(EventLoop* loop);
  ~EPoller();

  Timestamp poll(int timeoutMs, ChannelList* activeChannels);

  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);

  void AssertInLoopThread();

 private:
  static const int kiniteventlistsize = 16;

  void FillActiveChannels(int numevents,
                          ChannelList* activechannels) const;
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;
  typedef std::map<int, Channel*> ChannelMap;

  EventLoop* ownerloop_;
  int epollfd_;
  EventList events_;
  ChannelMap channels_;
};

#endif