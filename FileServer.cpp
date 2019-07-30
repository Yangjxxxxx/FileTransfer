#include "FileServer.h"
#include "EventLoop.h"

#define kBufSize (64*1024)

#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

using namespace smallMuduo;

typedef std::shared_ptr<FILE> FilePtr;

FileServer::FileServer(EventLoop* loop,
		const struct sockaddr_in& listenAddr)
	:server_(loop,listenAddr,"FileServer")
{
	server_.setConnectionCallback(std::bind(&FileServer::onConnection,this,std::placeholders::_1));
	server_.setMessageCallback(std::bind(&FileServer::onMessage,this,std::placeholders::_1,std::placeholders::_2));
	server_.setWriteCompleteCallback(std::bind(&FileServer::onWriteComplete,this,std::placeholders::_1));
}

void FileServer::setThreadNums(const int nums)
{
	assert(0<=nums);
	server_.setThreadNum(nums);
}

void FileServer::start()
{
	std::cout << " FileServer started.\n ";
	server_.start();	
}

void FileServer::onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
	std::cout << "HighWaterMark " << len << "\n";
}

//const int kBufSize = 64*1024;
//const char* g_file = NULL;
//typedef std::shared_ptr<FILE> FilePtr;
void FileServer::onMessage(const TcpConnectionPtr& conn,Buffer* buf)
{
	std::string msg(buf->retrieveAllAsString());
	std::cout << conn->name() << " receives " << msg.size() << " bytes at "
		<< " download file : " << msg << "\n";
//	muduo::string path = "/home/yangjx/workspace/ftp/" + msg;
//	const char* f_file = msg.c_str();
	msg.erase(msg.end()-1);

	std::cout << "msg : " << msg <<"size: "<<msg.size() <<"\n";
	if (conn->connected())
	{
		std::cout << "FileServer - Sending file  " << msg
			<< " to " << std::string(inet_ntoa(conn->peerAddress().sin_addr))
			<< ":" << ntohs(conn->peerAddress().sin_port) << "\n";
		conn->setHighWaterMarkCallback(std::bind(&FileServer::onHighWaterMark,this,std::placeholders::_1,std::placeholders::_2), kBufSize+1);

		FILE* fp = ::fopen(msg.c_str(), "rb");
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
			std::cout << "FileServer - no such file\n";
		}
	}
}

void FileServer::onConnection(const TcpConnectionPtr& conn)
{
	std::cout << "FileServer - " << std::string(inet_ntoa(conn->peerAddress().sin_addr)) << ":" << ntohs(conn->peerAddress().sin_port) 
	<< " -> " << std::string(inet_ntoa(conn->localAddress().sin_addr)) << ":" << ntohs(conn->localAddress().sin_port) 
	<< " is " << (conn->connected() ? "UP" : "DOWN") <<"\n";
  /*
  if (conn->connected())
  {
    LOG_INFO << "FileServer - Sending file  " << g_file
             << " to " << conn->peerAddress().toIpPort();
    conn->setHighWaterMarkCallback(std::bind(&FileServer::onHighWaterMark,this,_1,_2), kBufSize+1);

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
      LOG_INFO << "FileServer - no such file";
    }
  }
  */
}

void FileServer::onWriteComplete(const TcpConnectionPtr& conn)
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
	std::cout << "FileServer - done\n";
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
