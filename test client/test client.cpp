// test client.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "network.h"
#include "statemachine.h"
#define HOST_IP	"10.74.120.158"
#define CONTROL_PORT "5201"

int _tmain(int argc, _TCHAR* argv[])
{
	int socket;
	if (init_socket())
		fprintf(stderr,"init socket error\n");
	socket = tcp_connect(HOST_IP,CONTROL_PORT);
	if(socket != -1){
		CMDHandler(socket);
	}else
		fprintf(stderr,"connect to host err\n");
	return 0;
}

