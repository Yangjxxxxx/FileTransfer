/*************************************************************************
	> File Name: Poller.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Mon 22 Jul 2019 11:34:18 PM PDT
 ************************************************************************/
#ifndef POLLER_H
#define POLLER_H

#include "noncopyable.h"
#include "EventLoop.h"

#include <map>
#include <vector>

struct epoll_event;

namespace smallMuduo
{

class Channel;

class Poller:noncopyable
{
	public:
		typedef std::vector<Channel*> ChannelList;

		Poller(EventLoop* loop);
		~Poller();

		void poll(int timeoutMs,ChannelList* activeChannels);
		void updateChannel(Channel* channel);
		void removeChannel(Channel* channel);
		bool hasChannel(Channel* channel);

		void assertInLoopThread() const
		{
			ownerLoop_->assertInLoopThread();
		}

	private:
		static const int kInitEventListSize = 16;

		void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;
		void update(int operation,Channel* channel);

		typedef std::vector<struct epoll_event> EventList;

		int epollfd_;
		EventList events_;
		EventLoop* ownerLoop_;

	protected:
		typedef std::map<int,Channel*> ChannelMap;
		ChannelMap channels_;
};
}
#endif
