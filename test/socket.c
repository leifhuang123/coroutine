#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "socket.h"

/* set fd to nonblock */
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int fill_sockaddr_in(struct sockaddr_in *addr, const char *ip, int port)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    return inet_pton(AF_INET, ip, &(addr->sin_addr));
}

int start_tcp_server(const char *ip, int port, int backlog)
{
    struct sockaddr_in addr;
    fill_sockaddr_in(&addr, ip, port);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd > 0);
    
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    assert(ret != -1);
    
    ret = listen(sockfd, backlog);
    assert(ret != -1);
    
    return sockfd;
}

int start_tcp_client(const char *ip, int port)
{
    struct sockaddr_in addr;
    fill_sockaddr_in(&addr, ip, port);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd > 0);
    
    int ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0){
        perror("connect error");
        return -1;
    }
    return sockfd;
}

int start_udp_server(const char *ip, int port)
{
    struct sockaddr_in addr;
    fill_sockaddr_in(&addr, ip, port);
    
    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(sockfd > 0);
    
    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    assert(ret != -1);
        
    return sockfd;
}

int start_udp_client()
{
    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(sockfd > 0);
    
    return sockfd;
}
