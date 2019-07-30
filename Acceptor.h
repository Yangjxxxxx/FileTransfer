/*************************************************************************
	> File Name: Acceptor.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Wed 24 Jul 2019 11:11:30 PM PDT
 ************************************************************************/
#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "noncopyable.h"
#include "Channel.h"

#include <netinet/in.h>
#include <functional>

namespace smallMuduo
{

class EventLoop;

class Acceptor:noncopyable	
{
	public:
		typedef std::function<void(int sockfd,const struct sockaddr_in& addr)> NewConnectionCallback;

		Acceptor(EventLoop* loop,const struct sockaddr_in* listenAddr,bool reuseport);
		~Acceptor();

		void setNewConnectionCallback(const NewConnectionCallback& cb){newConnectionCallback_ = cb;}

		bool listenning() const {return listenning_;}
		void listen();

	private:
		void handleRead();

		EventLoop* loop_;
		int sockfd_;
		Channel acceptorChannel_;
		NewConnectionCallback newConnectionCallback_;
		bool listenning_;
};

}
#endif

