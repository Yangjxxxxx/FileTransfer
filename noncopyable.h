/*************************************************************************
	> File Name: noncopyable.h
	> Author: yangjx
	> Mail: 704911010@qq.com 
	> Created Time: Wed 03 Jul 2019 05:32:43 AM PDT
 ************************************************************************/
#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H


class noncopyable
{
	private:
		noncopyable(const noncopyable&);
		noncopyable& operator=(const noncopyable&);
	protected:
		noncopyable() = default;
		~noncopyable() = default;

};


#endif
