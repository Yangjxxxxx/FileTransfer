#include "QueryServer.h"
#include "EventLoop.h"

#define kBufSize (64*1024)

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>

using namespace smallMuduo;

typedef std::shared_ptr<FILE> FilePtr;

QueryServer::QueryServer(EventLoop* loop,
		const struct sockaddr_in& listenAddr,
		char* file)
	:server_(loop,listenAddr,"QueryServer"),
	g_file(file)
{
	server_.setConnectionCallback(std::bind(&QueryServer::onConnection,this,std::placeholders::_1));
	server_.setWriteCompleteCallback(std::bind(&QueryServer::onWriteComplete,this,std::placeholders::_1));
}

void QueryServer::start()
{
	std::cout << " QueryServer started.\n ";
	server_.start();	
}

void QueryServer::onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
	std::cout << "HighWaterMark " << len << std::endl;
}

//const int kBufSize = 64*1024;
//const char* g_file = NULL;
//typedef std::shared_ptr<FILE> FilePtr;

void QueryServer::onConnection(const TcpConnectionPtr& conn)
{
	std::cout << "QueryServer - " << std::string(inet_ntoa(conn->peerAddress().sin_addr)) << ":" << ntohs(conn->peerAddress().sin_port) 
	<< " -> " << std::string(inet_ntoa(conn->localAddress().sin_addr)) << ":" << ntohs(conn->localAddress().sin_port) 
	<< " is " << (conn->connected() ? "UP" : "DOWN") <<"\n";
	if (conn->connected())
	{
		std::cout<< "QueryServer - Sending file information " << g_file
			<< " to " << std::string(inet_ntoa(conn->peerAddress().sin_addr)) 
			<< ":" << ntohs(conn->peerAddress().sin_port) << "\n"; 

		conn->setHighWaterMarkCallback(std::bind(&QueryServer::onHighWaterMark,this,std::placeholders::_1,std::placeholders::_2), kBufSize+1);

		FILE* fp = ::fopen(g_file, "rb");
		if (fp)
		{
			FilePtr ctx(fp, ::fclose);
			conn->setContext(ctx);
			char buf[kBufSize];
			size_t nread = ::fread(buf, 1, sizeof buf, fp);
			conn->send(buf, static_cast<int>(nread));
		}
		else
		{
			conn->shutdown();
			std::cout << "QueryServer - no such file";
		}
	}
}

void QueryServer::onWriteComplete(const TcpConnectionPtr& conn)
{
	const FilePtr& fp = boost::any_cast<const FilePtr&>(conn->getContext());
	char buf[kBufSize];
	size_t nread = ::fread(buf, 1, sizeof buf, fp.get());
	if (nread > 0)
	{
		conn->send(buf, static_cast<int>(nread));
	}
	else
	{
		conn->forceClose();
		std::cout << "QueryServer - done\n";
	}
}

/*
int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    g_file = argv[1];

    EventLoop loop;
    InetAddress listenAddr(2021);
	QueryServer queryServer(&loop,listenAddr);
    queryServer.start();
    loop.loop();
  }
  else
  {
    fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
  }
}
*/
