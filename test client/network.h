#ifndef  NETWORK_H
#define NETWORK_H			1
#include <winsock2.h>
#include <ws2tcpip.h>
int init_socket();
int tcp_connect(char *hostname, char *service);
int recv_msg(SOCKET sockfd,char * buf,int needLen);
int send_msg(SOCKET sockfd,char *buf,int expectLen);
#endif