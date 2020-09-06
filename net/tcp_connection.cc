#include "net/tcp_connection.h"

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <iostream>

#include "net/channel.h"
#include "net/event_loop.h"
#include "net/socket.h"
#include "net/sockets_ops.h"

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& namearg,
                             int sockfd,
                             const InetAddress& localaddr,
                             const InetAddress& peeraddr)
    : loop_(loop), name_(namearg), state_(kconnecting), socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), localaddr_(localaddr), peeraddr_(peeraddr) {
  std::cout << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << sockfd << std::endl;
  channel_->SetReadCallback(
      std::bind(&TcpConnection::HandleRead, this, _1));
  channel_->SetWriteCallback(
      std::bind(&TcpConnection::HandleWrite, this));
  channel_->SetCloseCallback(
      std::bind(&TcpConnection::HandleClose, this));
  channel_->SetErrorCallback(
      std::bind(&TcpConnection::HandleError, this));
  // socket_->SetKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  std::cout << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd() << std::endl;
            // << " state=" << StateToString() << std::endl;
  // assert(state_ == kdisconnected);
}

void TcpConnection::send(const std::string& message) {
  if (state_ == kconnected) {
    if (loop_->IsInLoopThread()) {
      SendInLoop(message);
    } else {
      loop_->RunInLoop(
          std::bind(&TcpConnection::SendInLoop, this, message));
    }
  }
}

void TcpConnection::SendInLoop(const std::string& message) {
  loop_->AssertInLoopThread();
  ssize_t nwrote = 0;
  if (!channel_->IsWriting() && outputbuffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), message.data(), message.size());
    if (nwrote >= 0) {
      if (implicit_cast<size_t>(nwrote) < message.size()) {
        std::cout << "I am going to write more data" << std::endl;
      } else if (writecompletecallback_) {
        loop_->QueueInLoop(std::bind(writecompletecallback_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        std::cout << "TcpConnection::SendInLoop" << std::endl;
      }
    }
  }

  assert(nwrote >= 0);
  if (implicit_cast<size_t>(nwrote) < message.size()) {
    outputbuffer_.append(message.data()+nwrote, message.size()-nwrote);
    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}

void TcpConnection::shutdown() {
  if (state_ == kconnected) {
    SetState(kdisconnecting);
    loop_->RunInLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
  }
}

void TcpConnection::ShutdownInLoop() {
  loop_->AssertInLoopThread();
  if (!channel_->IsWriting()) {
    socket_->ShutdownWrite();
  }
}

// void TcpConnection::SetTcpNoDelay(bool on){
//   socket_->SetTcpNoDelay(on);
// }

void TcpConnection::ConnectEstablished() {
  loop_->AssertInLoopThread();
  assert(state_ == kconnecting);
  SetState(kconnected);
  channel_->tie(shared_from_this());
  channel_->EnableReading();

  connectioncallback_(shared_from_this());
}

void TcpConnection::ConnectDestroyed() {
  loop_->AssertInLoopThread();
  assert(state_ == kconnected || state_ == kdisconnecting);
  SetState(kdisconnected);
  channel_->DisableAll();
  connectioncallback_(shared_from_this());

  loop_->RemoveChannel(get_pointer(channel_));
}

void TcpConnection::HandleRead(Timestamp receivetime) {
  int savederror = 0;
  ssize_t n = inputbuffer_.readFd(channel_->fd(), &savederror);
  if (n > 0) {
    messagecallback_(shared_from_this(), &inputbuffer_, receivetime);
  } else if (n == 0) {
    HandleClose();
  } else {
    errno = savederror;
    std::cout << "TcpConnection::HandleRead" << std::endl;
    HandleError();
  }
}

void TcpConnection::HandleWrite() {
  loop_->AssertInLoopThread();
  if (channel_->IsWriting()) {
    ssize_t n = ::write(channel_->fd(),
                        outputbuffer_.peek(),
                        outputbuffer_.readableBytes());
    if (n > 0) {
      outputbuffer_.retrieve(n);
      if (outputbuffer_.readableBytes() == 0) {
        channel_->DisableWriting();
        if (writecompletecallback_) {
          loop_->QueueInLoop(std::bind(writecompletecallback_, shared_from_this()));
        }
        if (state_ == kdisconnecting) {
          ShutdownInLoop();
        }
      } else {
        std::cout << "I am going to write more data" << std::endl;
      }
    } else {
      std::cout << "TcpConnection::handleWrite" << std::endl;
    }
  } else {
    std::cout << "Connection is down, no more writing" << std::endl;
  }
}

void TcpConnection::HandleClose() {
  loop_->AssertInLoopThread();
  std::cout << "TcpConnection::HandleClose state = " << state_ << std::endl;
  assert(state_ == kconnected || state_ == kdisconnecting);
  channel_->DisableAll();
  closecallback_(shared_from_this());
}

void TcpConnection::HandleError() {
  int err = sockets::GetSocketError(channel_->fd());
  std::cout << "TcpConnection::HandleError [" << name_ << "] - SO_ERROR = " << err << " " << strerror(err);
}