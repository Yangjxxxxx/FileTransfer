/*************************************************************************
	> File Name: EventLoopThread.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Tue 23 Jul 2019 09:19:38 PM PDT
 ************************************************************************/
#include "EventLoopThread.h"
#include "EventLoop.h"
#include <iostream>

using namespace smallMuduo;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,const std::string& name)
	:loop_(NULL),
	mutex_1(),
	mutex_2(),
	callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
	if(loop_ != NULL)
	{
		loop_->quit();
		thread_->join();
	}else if(thread_ != NULL)
	{
		thread_->join();
	}
}

EventLoop* EventLoopThread::startLoop()
{
	std::unique_ptr<std::thread> threadPtr(new std::thread(std::bind(&EventLoopThread::threadFunc,this)));
	thread_ = std::move(threadPtr);

	EventLoop* loop = NULL;

	{
	std::lock_guard<std::mutex> guard(mutex_1);
	while(loop_ == NULL)
	{
		std::mutex mut;
		std::unique_lock<std::mutex> lck(mut);
		condition_.wait(lck);
	}
	loop = loop_;
	}

	return loop;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;

	if(callback_)
	{
		callback_(&loop);
	}

	{
	std::lock_guard<std::mutex> guard(mutex_2);
	loop_ = &loop;
	condition_.notify_one();
	}

	loop.loop();

	mutex_2.lock();
	loop_ = NULL;
	mutex_2.unlock();
}
