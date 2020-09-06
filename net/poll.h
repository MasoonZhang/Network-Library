#ifndef NET_POLLER_H_
#define NET_POLLER_H_

#include <map>
#include <vector>

#include "base/noncopyable.h"
#include "base/timestamp.h"

struct pollfd;

class EventLoop;
class Channel;

class Poller : noncopyable {
 public:
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* loop);
  ~Poller();

  Timestamp poll(int timeoutms, ChannelList* activechannels);

  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);
  void AssertInLoopThread();

 private:
  void FillActiveChannels(int numevents, ChannelList* activechannels) const;

  typedef std::vector<struct pollfd> PollFdList;
  typedef std::map<int, Channel*> ChannelMap;

  EventLoop* ownerloop_;
  PollFdList pollfds_;
  ChannelMap channels_;
};

#endif