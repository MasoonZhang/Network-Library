#include "net/connector.h"

#include <errno.h>

#include <iostream>

#include "net/channel.h"
#include "net/event_loop.h"
#include "net/inet_address.h"
#include "net/sockets_ops.h"

const int Connector::kmaxretrydelayms;

Connector::Connector(EventLoop* loop, const InetAddress& serveraddr)
  : loop_(loop),
    serveraddr_(serveraddr),
    connect_(false),
    state_(kdisconnected),
    retrydelayms_(kinitretrydelayms) {
  std::cout << "ctor[" << this << "]" << std::endl;
}

Connector::~Connector() {
  std::cout << "dtor[" << this << "]" << std::endl;
  loop_->cancel(timerid_);
  assert(!channel_);
}

void Connector::start() {
  connect_ = true;
  loop_->RunInLoop(std::bind(&Connector::StartInLoop, this));
}

void Connector::StartInLoop() {
  loop_->AssertInLoopThread();
  assert(state_ == kdisconnected);
  if (connect_) {
    connect();
  } else {
    std::cout << "do not connect" << std::endl;
  }
}

void Connector::connect() {
  int sockfd = sockets::CreateNonblockingOrDie();
  int ret = sockets::connect(sockfd, serveraddr_.GetSockAddr());
  int savederrno = (ret == 0) ? 0 : errno;
  switch (savederrno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      retry(sockfd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      std::cout << "connect error in Connector::StartInLoop " << savederrno << std::endl;
      sockets::close(sockfd);
      break;

    default:
      std::cout << "Unexpected error in Connector::StartInLoop " << savederrno << std::endl;
      sockets::close(sockfd);
      break;
  }
}

void Connector::restart() {
  loop_->AssertInLoopThread();
  SetState(kdisconnected);
  retrydelayms_ = kinitretrydelayms;
  connect_ = true;
  StartInLoop();
}

void Connector::stop() {
  connect_ = false;
  loop_->cancel(timerid_);
}

void Connector::connecting(int sockfd) {
  SetState(kconnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->SetWriteCallback(
      std::bind(&Connector::HandleWrite, this));
  channel_->SetErrorCallback(
      std::bind(&Connector::HandleError, this));

  channel_->EnableWriting();
}

int Connector::RemoveAndResetChannel() {
  channel_->DisableAll();
  loop_->RemoveChannel(get_pointer(channel_));
  int sockfd = channel_->fd();
  loop_->QueueInLoop(std::bind(&Connector::ResetChannel, this));
  return sockfd;
}

void Connector::ResetChannel()
{
  channel_.reset();
}

void Connector::HandleWrite() {
  std::cout << "Connector::HandleWrite " << state_ << std::endl;

  if (state_ == kconnecting) {
    int sockfd = RemoveAndResetChannel();
    int err = sockets::GetSocketError(sockfd);
    if (err) {
      std::cout << "Connector::HandleWrite - SO_ERROR = "
               << err << " " << strerror(err) << std::endl;
      retry(sockfd);
    } else if (sockets::IsSelfConnect(sockfd)) {
      std::cout << "Connector::HandleWrite - Self connect" << std::endl;
      retry(sockfd);
    } else {
      SetState(kconnected);
      if (connect_) {
        newconnectioncallback_(sockfd);
      } else {
        sockets::close(sockfd);
      }
    }
  } else {
    assert(state_ == kdisconnected);
  }
}

void Connector::HandleError() {
  std::cout << "Connector::HandleError" << std::endl;
  assert(state_ == kconnecting);

  int sockfd = RemoveAndResetChannel();
  int err = sockets::GetSocketError(sockfd);
  std::cout << "SO_ERROR = " << err << " " << strerror(err) << std::endl;
  retry(sockfd);
}

void Connector::retry(int sockfd) {
  sockets::close(sockfd);
  SetState(kdisconnected);
  if (connect_) {
    std::cout << "Connector::retry - Retry connecting to "
             << serveraddr_.ToIpPort() << " in "
             << retrydelayms_ << " milliseconds. ";
    timerid_ = loop_->RunAfter(retrydelayms_/1000.0,
                               std::bind(&Connector::StartInLoop, this));
    retrydelayms_ = std::min(retrydelayms_ * 2, kmaxretrydelayms);
  }
  else {
    std::cout << "do not connect" << std::endl;
  }
}
