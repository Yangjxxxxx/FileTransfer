#include "EventLoopThreadPool.h"
#include "EventLoop.h"

#include <thread>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <assert.h>

using namespace smallMuduo; 
using namespace std;

void print(EventLoop* p = NULL)
{
  cout<<"main(): pid = "<<getpid()<<", tid = "<<this_thread::get_id()<<",loop = "<<p<<endl;
}

void init(EventLoop* p)
{
  cout<<"init(): pid = "<<getpid()<<", tid = "<<this_thread::get_id()<<",loop = "<<p<<endl;
}

int main()
{
  print();

  EventLoop loop;
  loop.runInLoop(std::bind(&EventLoop::quit, &loop));

  {
    printf("Single thread %p:\n", &loop);
    EventLoopThreadPool model(&loop, "single");
    model.setThreadNum(0);
    model.start(init);
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
  }

  {
    printf("Another thread:\n");
    EventLoopThreadPool model(&loop, "another");
    model.setThreadNum(1);
    model.start(init);
    EventLoop* nextLoop = model.getNextLoop();
    nextLoop->runInLoop(std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop == model.getNextLoop());
    assert(nextLoop == model.getNextLoop());
    ::sleep(3);
  }

  {
    printf("Three threads:\n");
    EventLoopThreadPool model(&loop, "three");
    model.setThreadNum(3);
    model.start(init);
    EventLoop* nextLoop = model.getNextLoop();
    nextLoop->runInLoop(std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop != model.getNextLoop());
    assert(nextLoop != model.getNextLoop());
    assert(nextLoop == model.getNextLoop());
  }

  loop.loop();
}

