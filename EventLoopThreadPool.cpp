/*************************************************************************
	> File Name: EventLoopThreadPool.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Wed 24 Jul 2019 08:10:36 PM PDT
 ************************************************************************/
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

#include <iostream>
#include <assert.h>

using namespace smallMuduo;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg)
	:baseLoop_(baseLoop),
	name_(nameArg),
	started_(false),
	numThreads_(0),
	next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	assert(!started_);
	baseLoop_->assertInLoopThread();

	started_ = true;

	for(int i = 0; i < numThreads_; ++i)
	{
		std::string threadName = name_ + std::to_string(i);
		EventLoopThread* t = new EventLoopThread(cb,threadName);
		threads_.push_back(std::unique_ptr<EventLoopThread>(t));
		loops_.push_back(t->startLoop());
	}
	if(numThreads_ == 0 && cb)
	{
		cb(baseLoop_);
	}
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
	baseLoop_->assertInLoopThread();
	assert(started_);
	EventLoop* loop = baseLoop_;

	if(!loops_.empty())
	{
		loop = loops_[next_];
		++next_;
		if(static_cast<size_t>(next_) >= loops_.size())
		{
			next_ = 0;
		}
	}
	
	return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
	baseLoop_->assertInLoopThread();
	assert(started_);
	if(loops_.empty())
	{
		return std::vector<EventLoop*> (1,baseLoop_);
	}
	else
	{
		return loops_;
	}
}

