/*************************************************************************
	> File Name: TcpConnection.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Thu 25 Jul 2019 03:00:13 AM PDT
 ************************************************************************/

#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"

#include <errno.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

using namespace smallMuduo;

void smallMuduo::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	std::cout << string(inet_ntoa(conn->localAddress().sin_addr)) << " -> "
            << string(inet_ntoa(conn->peerAddress().sin_addr)) << " is "
            << (conn->connected() ? "UP" : "DOWN") << std::endl;
}

void smallMuduo::defaultMessageCallback(const TcpConnectionPtr&,Buffer* buf)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
		const string& nameArg,
        int sockfd,
        const struct sockaddr_in localAddr,
        const struct sockaddr_in peerAddr)
	: loop_(loop),
	name_(nameArg),
    state_(kConnecting),
    reading_(true),
	sockfd_(sockfd),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024)
{
	 channel_->setReadCallback(
		std::bind(&TcpConnection::handleRead, this));
	 channel_->setWriteCallback(
		std::bind(&TcpConnection::handleWrite, this));
	 channel_->setCloseCallback(
		std::bind(&TcpConnection::handleClose, this));

  //set keepalive
  {
	  int optval = 1;
	  setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,
				&optval,static_cast<socklen_t>(sizeof optval));
  }
}

TcpConnection::~TcpConnection()
{
	assert(state_ == kDisconnected);
}

void TcpConnection::send(const void* data, int len)
{
	send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(message);
		}
		else
		{
			void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
			loop_->runInLoop(std::bind(fp,this,message.as_string()));
		}
	}
}

void TcpConnection::send(Buffer* buf)
{
	if (state_ == kConnected)
	{
		if (loop_->isInLoopThread())
		{
			sendInLoop(buf->peek(), buf->readableBytes());
			buf->retrieveAll();
		}
		else
		{
			void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
			loop_->runInLoop(std::bind(fp,this,buf->retrieveAllAsString()));
		}
	}
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
	loop_->assertInLoopThread();
	ssize_t nwrote = 0;
	size_t remaining = len;
	bool faultError = false;
	if (state_ == kDisconnected)
	{
		return;
	}
	// if no thing in output queue, try writing directly
	if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
	{
		nwrote = ::write(channel_->fd(), data, len);
		if (nwrote >= 0)
		{
			remaining = len - nwrote;
			if (remaining == 0 && writeCompleteCallback_)
			{	
				loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
			}
		}
		else // nwrote < 0
		{
			nwrote = 0;
			if (errno != EWOULDBLOCK)
			{
				std::cout << "TcpConnection::sendInLoop: read failed.\n";
				if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
				{
					faultError = true;
				}
			}	
		}	
	}	

	assert(remaining <= len);
	if (!faultError && remaining > 0)
	{
		size_t oldLen = outputBuffer_.readableBytes();
		if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
		{
			loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
		}
		outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
		if (!channel_->isWriting())
		{
			channel_->enableWriting();
		}
	}
}

void TcpConnection::shutdown()
{
	if (state_ == kConnected)
	{	
		setState(kDisconnecting);
		loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
	}
}

void TcpConnection::shutdownInLoop()
{
	loop_->assertInLoopThread();
	if (!channel_->isWriting())
	{
		if(::shutdown(sockfd_,SHUT_WR) < 0)
		{
			std::cout<<"TcpConnection shutdown failed.\n";
		}
	}
}

void TcpConnection::forceClose()
{
	if (state_ == kConnected || state_ == kDisconnecting)
	{
		setState(kDisconnecting);
		loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
	}
}

void TcpConnection::forceCloseInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kConnected || state_ == kDisconnecting)
	{
		handleClose();
	}
}

const char* TcpConnection::stateToString() const
{
	switch (state_)
	{
		case kDisconnected:
			return "kDisconnected";
		case kConnecting:
			return "kConnecting";
		case kConnected:
			return "kConnected";
		case kDisconnecting:
			return "kDisconnecting";
		default:
			return "unknown state";
	}
}

void TcpConnection::setTcpNoDelay(bool on)
{
	int optval = on ? 1 : 0;
	::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,static_cast<socklen_t>(sizeof optval));
}

void TcpConnection::startRead()
{
	loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
	loop_->assertInLoopThread();
	if (!reading_ || !channel_->isReading())
	{
		channel_->enableReading();
		reading_ = true;
	}
}

void TcpConnection::stopRead()
{
	loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
	loop_->assertInLoopThread();
	if (reading_ || channel_->isReading())
	{
		channel_->disableReading();
		reading_ = false;
	}
}

void TcpConnection::connectEstablished()
{
	loop_->assertInLoopThread();
	assert(state_ == kConnecting);
	setState(kConnected);
	channel_->enableReading();

	connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
	loop_->assertInLoopThread();
	if (state_ == kConnected)
	{
		setState(kDisconnected);
		channel_->disableAll();

		connectionCallback_(shared_from_this());
	}
	channel_->remove();
}

void TcpConnection::handleRead()
{
	loop_->assertInLoopThread();
	int savedErrno = 0;
	ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
	if (n > 0)
	{
		messageCallback_(shared_from_this(), &inputBuffer_);
	}
	else if (n == 0)
	{
		handleClose();
	}
	else
	{
		errno = savedErrno;
		std::cout << "TcpConnection::handleRead read failed.\n";
	}
}

void TcpConnection::handleWrite()
{
	loop_->assertInLoopThread();
	if (channel_->isWriting())
	{
		ssize_t n = ::write(channel_->fd(),outputBuffer_.peek(),outputBuffer_.readableBytes());
		if (n > 0)
		{
			outputBuffer_.retrieve(n);
			if (outputBuffer_.readableBytes() == 0)
			{	
				channel_->disableWriting();
				if (writeCompleteCallback_)
				{
					loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
				}
				if (state_ == kDisconnecting)
				{		
					shutdownInLoop();
				}
			}
		}		
		else
		{
			std::cout << "TcpConnection::handleWrite: write failed.\n";
		}
	}
	else
	{
		std::cout << "Connection fd = " << channel_->fd()<< " is down, no more writing\n";
	}
}

void TcpConnection::handleClose()
{
	loop_->assertInLoopThread();
	std::cout << "fd = " << channel_->fd() << " state = " << stateToString();
	assert(state_ == kConnected || state_ == kDisconnecting);
	setState(kDisconnected);
	channel_->disableAll();

	TcpConnectionPtr guardThis(shared_from_this());
	connectionCallback_(guardThis);
	closeCallback_(guardThis);
}

