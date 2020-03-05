#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int setnonblocking(int fd);

int fill_sockaddr_in(struct sockaddr_in *addr, const char *ip, int port);

int start_tcp_server(const char *ip, int port, int backlog);

int start_tcp_client(const char *ip, int port);

int start_udp_server(const char *ip, int port);

int start_udp_client();

#endif
