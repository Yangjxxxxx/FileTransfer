/*************************************************************************
	> File Name: TcpConnection_unittest.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Sun 28 Jul 2019 01:48:21 AM PDT
 ************************************************************************/
#include "Acceptor.h"
#include "EventLoop.h"
#include "TcpConnection.h"

#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace smallMuduo;

void onConnection(const TcpConnectionPtr& conn)
{
	std::cout<<"here.\n";
	if(conn->connected())
	{
		string buf = "yangjx\n";
		conn->send(buf.c_str(),7);
	}
	else
	{
		conn->shutdown();
	}
}

void onMessage(const TcpConnectionPtr& conn,Buffer* buf)
{
	std::string msg(buf->retrieveAllAsString());
	std::cout<<msg<<std::endl;
	conn->forceClose();
}

class Connection
{
	public:
		Connection(EventLoop* lo){loop = lo;};
		~Connection(){};
		void newConnection(int sockfd,const struct sockaddr_in& peerAddr);
	private:
		std::shared_ptr<TcpConnection> tcpConnection;
		EventLoop* loop;
};

void Connection::newConnection(int sockfd,const struct sockaddr_in& peerAddr)
{
	std::cout<<"newConnection(): accept a new connection from "<<std::string(inet_ntoa(peerAddr.sin_addr))<<":"<<ntohs(peerAddr.sin_port)<<std::endl;
	
	struct sockaddr_in localAddr;
	memset(&localAddr,0,sizeof localAddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof localAddr);
	getsockname(sockfd,(struct sockaddr*)&localAddr,&addrlen);
	tcpConnection = std::shared_ptr<TcpConnection>(new TcpConnection(loop,
			"conn1",
			sockfd,
			localAddr,
			peerAddr
			));
	tcpConnection->setConnectionCallback(onConnection);
	tcpConnection->setMessageCallback(onMessage);
	loop->runInLoop(std::bind(&TcpConnection::connectEstablished,tcpConnection));
}

void setListenAddr(struct sockaddr_in* addr,uint16_t port)
{
	memset(addr,0,sizeof *addr);
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(port);
}


int main()
{
	std::cout<<"main(): pid = "<<getpid()<<"\n";

	struct sockaddr_in listenAddr;
	setListenAddr(&listenAddr,9981);

	EventLoop loop;
	Connection connection(&loop);
	Acceptor acceptor(&loop,&listenAddr,true);
	acceptor.setNewConnectionCallback(std::bind(&Connection::newConnection,connection,std::placeholders::_1,std::placeholders::_2));
	acceptor.listen();

	loop.loop();
}



