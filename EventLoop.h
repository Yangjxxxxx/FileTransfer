/*************************************************************************
	> File Name: EventLoop.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Sun 14 Jul 2019 01:27:46 AM PDT
 ************************************************************************/
#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <boost/any.hpp>

#include "noncopyable.h"

namespace smallMuduo
{
class Channel;
class Poller;

typedef std::function<void()> Functor;

class EventLoop:noncopyable
{
	private:
		void abortNotInLoopThread();
		void handleRead();
		void doPendingFunctors();

		typedef std::vector<Channel*> ChannelList;
		std::atomic_bool looping_;
		std::atomic_bool quit_;
		std::atomic_bool eventHandling_;
		std::atomic_bool callingPendingFunctors_;
		const std::thread::id threadId_;
		std::unique_ptr<Poller> poller_;
		int wakeupFd_;
		std::unique_ptr<Channel> wakeupChannel_;
		boost::any context_;

		ChannelList activeChannels_;
		Channel* currentChannel_;

		std::vector<Functor> pendingFunctors_;
		
		mutable std::mutex mutex_;
	
	public:

		EventLoop();
		~EventLoop();

		void loop();
		void quit();

		void runInLoop(Functor cb);
		void queueInLoop(Functor cb);
		int queueSize() const;

		void wakeup();
		void updateChannel(Channel* channel);
		void removeChannel(Channel* channel);
		bool hasChannel(Channel* channel);

		void assertInLoopThread()
		{
			if(!isInLoopThread())
			{
			abortNotInLoopThread();
			}
		}
		bool isInLoopThread() const{return threadId_ == std::this_thread::get_id();}
		bool eventHandling() const{return eventHandling_;}

		void setContext(const boost::any& context){context_ = context;}
		const boost::any& getContext() const{return context_;}

		static EventLoop* getEventLoopOfCurrentThread();
};
}
#endif

