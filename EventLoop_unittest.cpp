#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#include "EventLoop.h"

using namespace std;
using namespace smallMuduo;

EventLoop* g_loop;

void callback()
{
	cout<<"callback(): pid = "<<getpid()<<", tid = "<<this_thread::get_id()<<endl;
	EventLoop anotherLoop;
}

void threadFunc()
{
	cout<<"threadFunc(): pid = "<<getpid()<<", tid = "<<this_thread::get_id()<<endl;
	
	assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
	EventLoop loop;
	assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
	loop.runInLoop(callback);
	cout<<"here\n";
	loop.loop();
}

int main()
{
	cout<<"main(): pid = "<<getpid()<<", tid = "<<this_thread::get_id()<<endl;

	assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
	EventLoop loop;
	assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

	thread thread(threadFunc);

	loop.loop();

}
