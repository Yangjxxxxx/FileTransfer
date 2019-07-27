/*************************************************************************
	> File Name: Acceptor.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Wed 24 Jul 2019 11:47:22 PM PDT
 ************************************************************************/
#include "Acceptor.h"
#include "EventLoop.h"

#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

using namespace smallMuduo;

Acceptor::Acceptor(EventLoop* loop,const struct sockaddr_in* listenAddr,bool reuseport)
	:loop_(loop),
	sockfd_(socket(listenAddr->sin_family,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP)),
	acceptorChannel_(loop,sockfd_),
	listenning_(false)
{
	if(sockfd_ < 0)
	{
		std::cout<<"Acceptor constructor: sockfd < 0\n";
		abort();
	}

	//set reuseaddr
	{
		int optval = 1;
		setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,static_cast<socklen_t>(sizeof optval));
	}

	//set reuseport
	{
		int optval = reuseport ? 1 : 0;
		int ret = setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,static_cast<socklen_t>(sizeof optval));
		if(ret < 0 && reuseport)
		{
			std::cout<<"Acceptor constructor:setsocket(reuseport) failed.\n";
			abort();
		}
	}

	// bind socket
	{
		int ret = bind(sockfd_,(const struct sockaddr*)listenAddr,static_cast<socklen_t>(sizeof(struct sockaddr_in)));
		if(ret < 0)
		{
			std::cout<<"Acceptor constructor: bind failed.\n";
			abort();
		}
	}

	acceptorChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
	acceptorChannel_.disableAll();
	acceptorChannel_.remove();
}

void Acceptor::listen()
{
	loop_->assertInLoopThread();
	listenning_ = true;
	
	//listen
	{
		int ret = ::listen(sockfd_,SOMAXCONN);
		if(ret < 0)
		{
			std::cout<<"Acceptor::listen(): listen failed.\n";
			abort();
		}
	}
	acceptorChannel_.enableReading();
}

void Acceptor::handleRead()
{
	loop_->assertInLoopThread();

	struct sockaddr_in addr;
	memset(&addr,0,sizeof addr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
	int connfd = accept4(sockfd_,(struct sockaddr*)&addr,
			&addrlen,SOCK_NONBLOCK | SOCK_CLOEXEC);
	
	if(connfd >= 0)
	{
		if(newConnectionCallback_)
		{
			newConnectionCallback_(connfd,addr);
		}
		else
		{
			close(connfd);
		}
	}
	else
	{
		std::cout<<"Acceptor::handleRead(): accept4 failed.\n";
	}

}

