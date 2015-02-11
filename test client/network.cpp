#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
int usbip_set_nodelay(int sockfd)
{
	const char val = 1;
	int ret;

	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
	if (ret < 0)
		fprintf(stderr,"Err:setsockopt TCP_NODELAY");

	return ret;
}

int usbip_set_keepalive(int sockfd)
{
	const char val = 1;
	int ret;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
	if (ret < 0)
		fprintf(stderr,"Err:setsockopt SO_KEEPALIVE");

	return ret;
}
int tcp_connect(char *hostname, char *service)
{
	struct addrinfo hints, *res, *res0;
	int sockfd;
	int err;


	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;

	/* get all possible addresses */
	err = getaddrinfo(hostname, service, &hints, &res0);
	if (err) {
		fprintf(stderr,"Err:%s %s: %s", hostname, service, gai_strerror(err));
		return -1;
	}

	/* try all the addresses */
	for (res = res0; res; res = res->ai_next) {
		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

		err = getnameinfo(res->ai_addr, res->ai_addrlen,
			hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
		if (err) {
			fprintf(stderr,"Err:%s %s: %s", hostname, service, gai_strerror(err));
			continue;
		}

		fprintf(stderr,"Info:trying %s port %s\n", hbuf, sbuf);

		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) {
			fprintf(stderr,"Err:socket");
			continue;
		}

		/* should set TCP_NODELAY for usbip */
		usbip_set_nodelay(sockfd);
		/* TODO: write code for heatbeat */
		//usbip_set_keepalive(sockfd);
		usbip_set_keepalive(sockfd);
		err = connect(sockfd, res->ai_addr, res->ai_addrlen);
		if (err < 0) {
			closesocket(sockfd);
			continue;
		}

		/* connected */
		fprintf(stderr,"Info:connected to %s:%s", hbuf, sbuf);
		freeaddrinfo(res0);
		return sockfd;
	}


	fprintf(stderr,"Info:%s:%s, %s", hostname, service, "no destination to connect to");
	freeaddrinfo(res0);

	return -1;
}
int recv_msg(SOCKET sockfd,char * buf,int needLen){
	int rdlen =0;
	int errNO;
	while(needLen>0){
		rdlen = recv(sockfd,buf,needLen,0);
		if(rdlen<=0){
			if(rdlen==0){
				//printf("ERROR:%s connect peer close this socket\n",__FUNCTION__);
				fprintf(stderr,"Err:connect peer close this socket\n");
			}
			else{
				errNO=WSAGetLastError();
				// printf("ERROR:windows error number is %d\n",errNO);
				fprintf(stderr,"Err:windows error number is %d\n",errNO);
				//err("\n");

			}
			return -1;
		}
		needLen -= rdlen;
		buf += rdlen;
	}
	return rdlen;
}
int send_msg(SOCKET sockfd,char *buf,int expectLen){
	int len,errNO;
	while(expectLen>0){
		len = send(sockfd,buf,expectLen,0);
		if(len<=0){
			errNO=WSAGetLastError();
			//printf("ERROR:windows error number is %d\n",errNO);
			err("windows error number is %d\n",errNO);

			return -1;
		}
		expectLen -= len;
		buf += len;
	}
	return 0;
}

int init_socket()
{
	WSADATA wsaData;
	int err;

	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		fprintf(stderr,"Error:WSAStartup failed with error: %d\n", err);
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		fprintf(stderr,"Error:Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return -1;
	}
	return 0;
}