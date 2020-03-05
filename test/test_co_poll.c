#define _GNU_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <poll.h>
#include "../src/coroutine.h"
#include "socket.h"

#define MAX_CO_NUM      10
#define CLIENT_LIMIT    10
#define MAXLINE         512

struct client{
    int sockfd;
    int co_id;
};

struct client clients[CLIENT_LIMIT] = {0};

void handle_conn(schedule_t *s, void *args)
{
    int connfd = *(int *)args;
    int n = 0;
    char buf[MAXLINE] = {0};
    while (1)
    {
        memset(buf, 0, MAXLINE);
        n = recv(connfd, buf, MAXLINE-1, MSG_DONTWAIT);
        if (n < 0)
        {
            coroutine_yield(s);
        }
        else if (n == 0)
        {
            printf("connection on fd %d closed\n", connfd);
            break;
        }
        else
        {
            buf[n] = '\0';
            printf("coroutine id: %d, connfd: %d, recv: %s\n", s->running_coroutine, connfd, buf);
            if (strcmp(buf, "exit") == 0)
                break;
            send(connfd, buf, strlen(buf), MSG_DONTWAIT);
            coroutine_yield(s);
        }
    }
    close(connfd);
}

int main()
{
    schedule_t *s = schedule_create(MAX_CO_NUM);
    
    int listenfd = start_tcp_server("0.0.0.0", 12345, MAX_CO_NUM);
    
    struct pollfd fds[CLIENT_LIMIT+1];
    int client_cnt = 0;
    /* init pollfd */
    int i;
    for (i=1; i<CLIENT_LIMIT; ++i){
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }
    /* register listenfd event */
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;
    
    while(1)
    {
        int ret = poll(fds, client_cnt+1, -1);
        if (ret < 0){
            perror("poll failed");
            break;
        }
        schedule_show(s);
        for (i=0; i<client_cnt+1; ++i){
            int sockfd = fds[i].fd;
            if ((fds[i].revents & POLLERR) || (fds[i].revents & POLLRDHUP))
            {
                printf("close connection on fd %d\n", sockfd);
                close(sockfd);
                coroutine_destroy(s, clients[sockfd].co_id);
                clients[sockfd] = clients[fds[client_cnt].fd];
                fds[i] = fds[client_cnt];
                --client_cnt;
                --i;
            }
            else if ((sockfd == listenfd) && (fds[i].revents & POLLIN))
            {
                struct sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                
                client_addrlen = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
                if (connfd < 0){
                    perror("accept error");
                    continue;
                }
                if (client_cnt >= CLIENT_LIMIT){
                    // close new coming connection
                    printf("too many clients\n");
                    close(connfd);
                    continue;
                }
                int id = coroutine_create(s, handle_conn, &connfd);
                if (id < 0)
                {
                    puts("coroutine_create failed");
                    close(connfd);
                    continue;
                }
                else
                {
                    printf("new coroutine %d\n", id);
                    ++client_cnt;
                    clients[connfd].sockfd = connfd;
                    clients[connfd].co_id = id;
                    setnonblocking(connfd);
                    fds[client_cnt].fd = connfd;
                    fds[client_cnt].events = POLLIN | POLLRDHUP | POLLERR;
                    fds[client_cnt].revents = 0;
                    coroutine_resume(s, id);
                }
            }
            else if (fds[i].revents & POLLIN)
            {
                coroutine_resume(s, clients[sockfd].co_id);
            }
        }
    }
    
    schedule_destroy(s);
    close(listenfd);
    return 0;
}
