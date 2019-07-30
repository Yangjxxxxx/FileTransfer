/*************************************************************************
	> File Name: TcpConnection_unittest.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Sun 29 Jul 2019 01:48:21 AM PDT
 ************************************************************************/
#include "EventLoop.h"
#include "TcpServer.h"

#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace smallMuduo;

void onConnection(const TcpConnectionPtr& conn)
{
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
	TcpServer server(&loop,listenAddr,"server");
	server.setConnectionCallback(onConnection);
	server.setMessageCallback(onMessage);
	server.start();

	loop.loop();
}



