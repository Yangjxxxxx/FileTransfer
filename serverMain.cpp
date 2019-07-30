#include "QueryServer.h"
#include "FileServer.h"
#include "EventLoop.h"

#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

using namespace smallMuduo;

char* file_ = NULL;

void setaddr(struct sockaddr_in* addr,uint16_t port)
{
	memset(addr,0,sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(port);
}

int main(int argc,char* argv[])
{
	std::cout << " pid = " << getpid() << "\n";
	if(argc > 1){
		file_ = argv[1];

		EventLoop loop;
		struct sockaddr_in listenAddr_1;
		setaddr(&listenAddr_1,3038);
		QueryServer queryServer(&loop,listenAddr_1,file_);

		struct sockaddr_in listenAddr_2;
		setaddr(&listenAddr_2,2021);
		FileServer fileServer(&loop,listenAddr_2);
		fileServer.setThreadNums(4);

		queryServer.start();
		fileServer.start();

		loop.loop();
	}
	else{
		fprintf(stderr,"Usage: %s file_for_downloading\n",argv[0]);
	}
}
