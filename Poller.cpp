/*************************************************************************
	> File Name: Poller.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Mon 22 Jul 2019 10:38:19 PM PDT
 ************************************************************************/
#include "Poller.h"
#include "Channel.h"

#include <poll.h>
#include <sys/epoll.h>
#include <iostream>

using namespace smallMuduo;

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

Poller::Poller(EventLoop* loop)
	:ownerLoop_(loop),
	epollfd_(epoll_create1(EPOLL_CLOEXEC)),
	events_(kInitEventListSize)
{
	if(epollfd_ < 0)
	{
		std::cout<<"Poller.cpp:Poller():epollfd_ < 0\n";
		abort();
	}
}

Poller::~Poller()
{
	close(epollfd_);
}

void Poller::poll(int timeoutMs,ChannelList* activeChannels)
{
	int numEvents = epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
	int savedErrno = errno;
	if(numEvents > 0)
	{
		fillActiveChannels(numEvents,activeChannels);
		if(static_cast<size_t>(numEvents) == events_.size())
		{
			events_.resize(events_.size()*2);
		}
	}
}

void Poller::fillActiveChannels(int numEvents,ChannelList* activeChannels) const
{
	for(int i = 0;i < numEvents;++i)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		channel->set_revents(events_[i].events);
		activeChannels->push_back(channel);
	}
}

void Poller::updateChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	const int index = channel->index();
	if(index == kNew || index == kDeleted)
	{
		int fd = channel->fd();
		if(index == kNew)
		{
			channels_[fd] = channel;
		}
		else
		{

		}
		channel->set_index(kAdded);
		update(EPOLL_CTL_ADD,channel);
	}
	else
	{
		int fd = channel->fd();
		(void)fd;
		if(channel->isNoneEvent())
		{
			update(EPOLL_CTL_DEL,channel);
			channel->set_index(kDeleted);
		}
		else
			update(EPOLL_CTL_MOD,channel);
	}
}

void Poller::removeChannel(Channel* channel)
{
	Poller::assertInLoopThread();
	int fd = channel->fd();
	int index = channel->index();
	size_t n = channels_.erase(fd);

	if(index = kAdded)
	{
		update(EPOLL_CTL_DEL,channel);
	}
	channel->set_index(kNew);
}

void Poller::update(int operation,Channel* channel)
{
	struct epoll_event event;
	memset(&event,0,sizeof(event));
	event.events = channel->events();
	event.data.ptr = channel;
	int fd = channel->fd();
	epoll_ctl(epollfd_,operation,fd,&event);
}

bool Poller::hasChannel(Channel* channel)
{
	assertInLoopThread();
	ChannelMap::const_iterator it = channels_.find(channel->fd());
	return it != channels_.end() && it->second == channel;
}
