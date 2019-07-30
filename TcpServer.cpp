/*************************************************************************
	> File Name: TcpServer.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Mon 29 Jul 2019 12:43:12 AM PDT
 ************************************************************************/
#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>

using namespace smallMuduo;

TcpServer::TcpServer(EventLoop* loop,
                     const struct sockaddr_in& listenAddr,
                     const string& nameArg)
    :loop_(loop),
    name_(nameArg),
	ipPort_(string(inet_ntoa(listenAddr.sin_addr)) + ":" + std::to_string(static_cast<int>(ntohs(listenAddr.sin_port)))),
	started_(false),
    acceptor_(new Acceptor(loop, &listenAddr, true)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),
    nextConnId_(1)
{
	acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
	std::cout<<name_<<" is constructed.\n";
}

TcpServer::~TcpServer()
{
	loop_->assertInLoopThread();
	std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing\n";

	for (auto& item : connections_)
	{
		TcpConnectionPtr conn(item.second);
		item.second.reset();
		conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
	}
}

void TcpServer::setThreadNum(int numThreads)
{
	assert(0 <= numThreads);
	threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
	if (!started_)
	{
		started_ = true;
		threadPool_->start(threadInitCallback_);

		assert(!acceptor_->listenning());
		loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
	}
}

void TcpServer::newConnection(int sockfd, const struct sockaddr_in& peerAddr)
{
	loop_->assertInLoopThread();
	EventLoop* ioLoop = threadPool_->getNextLoop();
	string connName = name_ + "-" + ipPort_ + std::to_string(nextConnId_);
	++nextConnId_;

	std::cout << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << std::string(inet_ntoa(peerAddr.sin_addr))
		   <<":"<<ntohs(peerAddr.sin_port)<<"\n";
	struct sockaddr_in localAddr;
	memset(&localAddr,0,sizeof localAddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof localAddr);
	getsockname(sockfd,(struct sockaddr*)&localAddr,&addrlen);
	TcpConnectionPtr conn(new TcpConnection(ioLoop, connName,sockfd,localAddr,peerAddr));
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	std::cout << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();
	size_t n = connections_.erase(conn->name());
	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

