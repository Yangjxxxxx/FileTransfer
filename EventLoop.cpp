/*************************************************************************
	> File Name: EventLoop.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Sun 14 Jul 2019 02:45:23 AM PDT
 ************************************************************************/
#include <utility>
#include <sys/eventfd.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"

using namespace smallMuduo;

namespace
{

__thread EventLoop* t_loopInThisThread = NULL;

const int kPollTimeMs = 10000;

int createEventFd()
{
	int evtfd = eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
	if(evtfd < 0)
	{
		abort();
	}
	return evtfd;
}

class IgnoreSigPipe
{
	public:
		IgnoreSigPipe()
		{
			::signal(SIGPIPE,SIG_IGN);
		}
};
IgnoreSigPipe ignoreSigPipe;
}//namespace

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

EventLoop::EventLoop()
	:looping_(false),
	quit_(false),
	eventHandling_(false),
	callingPendingFunctors_(false),
	threadId_(std::this_thread::get_id()),
	poller_(new Poller(this)),
	wakeupFd_(createEventFd()),
	wakeupChannel_(new Channel(this,wakeupFd_)),
	currentChannel_(NULL)
{
	if(!t_loopInThisThread)
	{
		t_loopInThisThread = this;
	}
	else{
		std::cout<<" Another EventLoop "<< t_loopInThisThread << " exits in this thread "<<threadId_<<"\n";
		abort();
	}
	wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
	std::cout<<"EventLoop "<<this<<" of thread "<<threadId_<<" destructs in thread "<< std::this_thread::get_id()<<"\n";
	wakeupChannel_->disableAll();
	wakeupChannel_->remove();
	close(wakeupFd_);
	t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;
	quit_ = false;
	std::cout << "EventLoop " << this << " start looping.\n";

	while(!quit_)
	{
		activeChannels_.clear();
		poller_->poll(kPollTimeMs,&activeChannels_);

		eventHandling_ = true;
		for(Channel* channel : activeChannels_)
		{
			currentChannel_ = channel;
			currentChannel_->handleEvent();
		}

		currentChannel_ = NULL;
		eventHandling_ = false;
		doPendingFunctors();
	}

	std::cout<<"EventLoop " << this << " stop looping.\n";
	looping_ = false;
}

void EventLoop::quit()
{
	quit_ = true;
	if(!isInLoopThread())
	{
		wakeup();
	}
}

void EventLoop::runInLoop(Functor cb)
{
	if(isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(std::move(cb));
	}
}

void EventLoop::queueInLoop(Functor cb)
{
	{
		mutex_.lock();
		pendingFunctors_.push_back(std::move(cb));
		mutex_.unlock();
	}

	if(!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup();
	}
}

int EventLoop::queueSize() const
{
	mutex_.lock();
	int size = pendingFunctors_.size();
	mutex_.unlock();
	
	return size;
}

void EventLoop::updateChannel(Channel* channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	//assert((channel->ownerLoop) == this);
	assertInLoopThread();
	if(eventHandling_)
	{
		assert(currentChannel_ == channel || std::find(activeChannels_.begin(),activeChannels_.end(),channel) == activeChannels_.end());
	}
	poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
	//assert(channel->ownerLoop == this);
	assertInLoopThread();
	return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
	std::cout<<"EventLoop::abortNotInLoopThread - EventLoop "<<this<<" was create in threadId = "<<threadId_<<",current thread id = "<<std::this_thread::get_id();
	abort();
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = write(wakeupFd_,&one,sizeof one);
	if(n != sizeof one)
	{
		std::cout<<"EventLoop.cpp: wakeup():abort()\n";
		abort();
	}
}

void EventLoop::handleRead()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_,&one,sizeof one);
	if(n != sizeof one)
		abort();
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;

	{
		mutex_.lock();
		functors.swap(pendingFunctors_);
		mutex_.unlock();
	}

	for(const Functor& functor : functors)
	{
		functor();
	}
	callingPendingFunctors_ = false;
}
