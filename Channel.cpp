/*************************************************************************
	> File Name: Channel.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Tue 16 Jul 2019 08:13:20 PM PDT
 ************************************************************************/

#include <poll.h>
#include <assert.h>

#include "EventLoop.h"
#include "Channel.h"

using namespace smallMuduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop,int fd)
	:loop_(loop),
	fd_(fd),
	events_(0),
	revents_(0),
	index_(-1),
	eventHandling_(false),
	addedToLoop_(false)
{
}

Channel::~Channel()
{
	assert(!eventHandling_);
	assert(!addedToLoop_);
	if(loop_->isInLoopThread())
	{
		assert(!loop_->hasChannel(this));
	}
}

void Channel::update()
{
	addedToLoop_ = true;
	loop_->updateChannel(this);
}

void Channel::remove()
{
	assert(isNoneEvent());
	addedToLoop_ = false;
	loop_->removeChannel(this);
}

void Channel::handleEvent()
{
	eventHandling_ = true;
	if((revents_ & POLLHUP) && !(revents_ & POLLIN))
	{
		if(closeCallback_)
			closeCallback_();
	}
	if(revents_ & POLLOUT)
	{
		if(writeCallback_)
			writeCallback_();
	}
	if(revents_ & (POLLIN | POLLPRI | POLLRDHUP))
	{
		if(readCallback_)
			readCallback_();
	}
	eventHandling_ = false;
}


