#ifndef QUERYSERVER_H
#define QUERYSERVER_H

#include "TcpServer.h"

using namespace smallMuduo;

class QueryServer{
	public:
		QueryServer(EventLoop* loop,
				const struct sockaddr_in& listenAddr,
				char* file);

		void start();
	
	private:
		void onHighWaterMark(const TcpConnectionPtr& conn, size_t len);
		void onConnection(const TcpConnectionPtr& conn);
		void onWriteComplete(const TcpConnectionPtr& conn);

		TcpServer server_;
		const char* g_file;
};

#endif //QUERYSERVER
