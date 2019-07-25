/*************************************************************************
	> File Name: TcpConnection.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Thu 25 Jul 2019 02:19:54 AM PDT
 ************************************************************************/
#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "noncopyable.h"

#include <muduo/net/Buffer.h>
#include <muduo/base/StringPiece.h>
#include <memory>
#include <functional>
#include <boost/any.hpp>
#include <netinet/in.h>

struct tcp_info;

using muduo::net::Buffer;
using std::string;
using muduo::StringPiece;

namespace smallMuduo
{

class Channel;
class EventLoop;

class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&,size_t)> HighWaterMarkCallback;
typedef std::function<void(const TcpConnectionPtr&,Buffer*)> MessageCallback;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
	public:

		TcpConnection(EventLoop* loop,
				const string& nameArg,
				int sockfd,
				const struct sockaddr_in localAddr,
				const struct sockaddr_in peerAddr);
		~TcpConnection();

		EventLoop* getLoop() const { return loop_; }
		const string& name() const { return name_; }
		struct sockaddr_in localAddress() const { return localAddr_; }
		struct sockaddr_in peerAddress() const { return peerAddr_; }
		bool connected() const { return state_ == kConnected; }
		bool disconnected() const { return state_ == kDisconnected; }

		// void send(string&& message); // C++11
		void send(const void* message, int len);
		void send(const StringPiece& message);
		// void send(Buffer&& message); // C++11
		void send(Buffer* message);  // this one will swap data
		void shutdown(); // NOT thread safe, no simultaneous calling
		void forceClose();
		void forceCloseWithDelay(double seconds);
		void setTcpNoDelay(bool on);
		void startRead();
		void stopRead();
		bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

		void setContext(const boost::any& context)
		{ context_ = context; }

		const boost::any& getContext() const
		{ return context_; }

		boost::any* getMutableContext()
		{ return &context_; }

		void setConnectionCallback(const ConnectionCallback& cb)
		{ connectionCallback_ = cb; }

		void setMessageCallback(const MessageCallback& cb)
		{ messageCallback_ = cb; }

		void setWriteCompleteCallback(const WriteCompleteCallback& cb)
		{ writeCompleteCallback_ = cb; }

		void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
		{ highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

		Buffer* inputBuffer()
		{ return &inputBuffer_; }

		Buffer* outputBuffer()
		{ return &outputBuffer_; }

		void setCloseCallback(const CloseCallback& cb)
		{ closeCallback_ = cb; }

		// called when TcpServer accepts a new connection
		void connectEstablished();   // should be called only once
		// called when TcpServer has removed me from its map
		void connectDestroyed();  // should be called only once

	private:
		enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
		void handleRead();
		void handleWrite();
		void handleClose();
		// void sendInLoop(string&& message);
		void sendInLoop(const StringPiece& message);
		void sendInLoop(const void* message, size_t len);
		void shutdownInLoop();
		// void shutdownAndForceCloseInLoop(double seconds);
		void forceCloseInLoop();
		void setState(StateE s) { state_ = s; }
		const char* stateToString() const;
		void startReadInLoop();
		void stopReadInLoop();

		EventLoop* loop_;
		const string name_;
		StateE state_;  // FIXME: use atomic variable
		bool reading_;
		// we don't expose those classes to client.
		int sockfd_;
		std::unique_ptr<Channel> channel_;
		const struct sockaddr_in localAddr_;
		const struct sockaddr_in peerAddr_;
		ConnectionCallback connectionCallback_;
		MessageCallback messageCallback_;
		WriteCompleteCallback writeCompleteCallback_;
		HighWaterMarkCallback highWaterMarkCallback_;
		CloseCallback closeCallback_;
		size_t highWaterMark_;
		Buffer inputBuffer_;
		Buffer outputBuffer_; 
		boost::any context_;
};

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn,Buffer* buffer);

}  // namespace smallMuduo

#endif
