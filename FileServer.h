#ifndef FILESERVER_H
#define FILESERVER_H

#include "TcpServer.h"

using namespace smallMuduo;

class FileServer{
	public:
		FileServer(EventLoop* loop,
				const struct sockaddr_in& listenAddr);

		void start();
		void setThreadNums(const int nums);

	private:
		void onHighWaterMark(const TcpConnectionPtr& conn, size_t len);
		void onConnection(const TcpConnectionPtr& conn);
		void onWriteComplete(const TcpConnectionPtr& conn);
		void onMessage(const TcpConnectionPtr& conn,
				Buffer* buf);
		
		TcpServer server_;
};

#endif //FILESERVER_H
