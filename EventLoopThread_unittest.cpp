#include "EventLoopThread.h"
#include "EventLoop.h"

#include <thread>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace smallMuduo;

void print(EventLoop* p = NULL)
{
	cout<<"print: pid = "<<getpid()<<",tid = "<<this_thread::get_id()<<",loop = "<<p<<endl;
}

void quit(EventLoop* p)
{
	print(p);
	p->quit();
}

int main()
{
	print();
	{
		EventLoopThread thr1;
	}

	{
		EventLoopThread thr2;
		EventLoop* loop = thr2.startLoop();
		loop->runInLoop(std::bind(print,loop));
		sleep(2);
	}

	{
		EventLoopThread thr3;
		EventLoop* loop = thr3.startLoop();
		loop->runInLoop(std::bind(quit,loop));
		sleep(2);
	}
	return 0;
}

