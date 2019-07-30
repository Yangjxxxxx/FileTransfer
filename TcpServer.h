/*************************************************************************
	> File Name: TcpServer.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Mon 29 Jul 2019 12:30:24 AM PDT
 ************************************************************************/
#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "TcpConnection.h"
#include "noncopyable.h"

#include <map>
#include <atomic>
#include <netinet/in.h>

using std::string;

namespace smallMuduo
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer : noncopyable
{
	public:
		typedef std::function<void(EventLoop*)> ThreadInitCallback;

		TcpServer(EventLoop* loop,
				const struct sockaddr_in& listenAddr,
				const string& nameArg);
		~TcpServer(); 

		const string& ipPort() const { return ipPort_; }
		const string& name() const { return name_; }
		EventLoop* getLoop() const { return loop_; }

		void setThreadNum(int numThreads);
		void setThreadInitCallback(const ThreadInitCallback& cb)
		{ threadInitCallback_ = cb; }
		std::shared_ptr<EventLoopThreadPool> threadPool()
		{ return threadPool_; }

		void start();

		void setConnectionCallback(const ConnectionCallback& cb)
		{ connectionCallback_ = cb; }

		void setMessageCallback(const MessageCallback& cb)
		{ messageCallback_ = cb; }

		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{ writeCompleteCallback_ = cb; }

	private:
		void newConnection(int sockfd, const struct sockaddr_in& peerAddr);
		void removeConnection(const TcpConnectionPtr& conn);
		void removeConnectionInLoop(const TcpConnectionPtr& conn);

		typedef std::map<string, TcpConnectionPtr> ConnectionMap;

		EventLoop* loop_;  // the acceptor loop
		const string ipPort_;
		const string name_;
		std::unique_ptr<Acceptor> acceptor_;
		std::shared_ptr<EventLoopThreadPool> threadPool_;
		ConnectionCallback connectionCallback_;
		MessageCallback messageCallback_;
		WriteCompleteCallback writeCompleteCallback_;
		ThreadInitCallback threadInitCallback_;
		std::atomic_bool started_;
		int nextConnId_;
		ConnectionMap connections_;
};

}
#endif
