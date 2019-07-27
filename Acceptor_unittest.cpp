/*************************************************************************
	> File Name: Acceptor_unittest.cpp
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Sat 27 Jul 2019 03:41:02 AM PDT
 ************************************************************************/
#include "Acceptor.h"
#include "EventLoop.h"

#include<iostream>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
using namespace smallMuduo;

void newConnection(int sockfd,const struct sockaddr_in& peerAddr)
{
	std::cout<<"newConnection(): accept a new connection from "<<std::string(inet_ntoa(peerAddr.sin_addr))<<":"<<ntohs(peerAddr.sin_port)<<std::endl;
	write(sockfd,"how are you?\n",13);
	close(sockfd);
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
	Acceptor acceptor(&loop,&listenAddr,true);
	acceptor.setNewConnectionCallback(newConnection);
	acceptor.listen();

	loop.loop();
}

