/*************************************************************************
	> File Name: EventLoopThread.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Tue 23 Jul 2019 09:08:39 PM PDT
 ************************************************************************/
#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>

#include "noncopyable.h"

namespace smallMuduo
{

class EventLoop;

class EventLoopThread:noncopyable
{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;
		
		EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),const std::string& name = std::string());
		~EventLoopThread();
		EventLoop* startLoop();

	private:
		void threadFunc();

		EventLoop* loop_;
		std::mutex mutex_1;
		std::mutex mutex_2;
		std::condition_variable condition_;
		std::unique_ptr<std::thread> thread_;
		ThreadInitCallback callback_;
};


}


#endif
