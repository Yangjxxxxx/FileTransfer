/*************************************************************************
	> File Name: EventLoopThreadPool.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Wed 24 Jul 2019 07:58:49 PM PDT
 ************************************************************************/
#ifndef EVENTLOOPTHREADPOOL_H
#define EVENTLOOPTHREADPOOL_H

#include "noncopyable.h"

#include <functional>
#include <memory>
#include <vector>

namespace smallMuduo
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool:noncopyable
{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;

		EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);
		~EventLoopThreadPool();

		void setThreadNum(int numThreads){numThreads_ = numThreads;}
		void start(const ThreadInitCallback& cb = ThreadInitCallback());

		EventLoop* getNextLoop();

		std::vector<EventLoop*> getAllLoops();

		bool started() const{return started_;}

		const std::string& name()const {return name_;}

	private:

		EventLoop* baseLoop_;
		std::string name_;
		bool started_;
		int numThreads_;
		int next_;
		std::vector<std::unique_ptr<EventLoopThread>> threads_;
		std::vector<EventLoop*> loops_;

};
}

#endif
