/*************************************************************************
	> File Name: Channel.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Tue 16 Jul 2019 07:49:40 PM PDT
 ************************************************************************/
#ifndef SMALLMUDUO_CHANNEL_H
#define SMALLMUUDO_CHANNEL_H

#include <functional>
#include <memory>

namespace smallMuduo
{

class EventLoop;

class Channel:noncopyable
{
	public:
		typedef std::function<void()> EventCallback;

		Channel(EventLoop* loop,int fd);
		~Channel();
		
		void handleEvent();
		void setReadCallback(EventCallback cb){readCallback_ = std::move(cb);}
		void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb);}
		void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb);}

		int fd() const{return fd_;}
		int events() const{return events_;}
		void set_revents(int revt){revents_ = revt;}
		bool isNoneEvent() const{return events_ == kNoneEvent;}

		void enableReading() {events_ |= kReadEvent;update();}
		void disableReading() {events_ &= ~kReadEvent;update();}
		void enableWriting() {events_ |= kWriteEvent;update();}
		void disableWriting() {events_ &= ~kWriteEvent;update();}
		void disableAll() {events_ = kNoneEvent;update();}
		bool isWriting() const {return events_ & kWriteEvent;}
		bool isReading() const {return events_ & kReadEvent;}

		int index(){return index_;}
		void set_index(int idx){index_ = idx;}

		EventLoop* ownerLoop() {return loop_;}
		void remove();

	private:
		void update();
		
		static const int kNoneEvent;
		static const int kReadEvent;
		static const int kWriteEvent;

		EventLoop* loop_;
		const int fd_;
		int events_;
		int revents_;
		int index_;

		bool eventHandling_;
		bool addedToLoop_;
		EventCallback readCallback_;
		EventCallback writeCallback_;
		EventCallback closeCallback_;

};

}


#endif
