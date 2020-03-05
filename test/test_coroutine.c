#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ucontext.h>
#include <sys/time.h>
#include "../src/coroutine.h"
#include "socket.h"

void test_getcontext()
{
    ucontext_t context;
    getcontext(&context);
    puts("hello world");
    sleep(1);
    setcontext(&context);
}

void test_func1()
{
    // printf("%s\n", __func__);
    return;
}

void test_func2(ucontext_t *ctx)
{
    ucontext_t cur_ctx;
    printf("%s\n", __func__);
    swapcontext(&cur_ctx, ctx);
}

void test_swapcontext()
{
    ucontext_t main_ctx, ctx1, ctx2;
    char stack[1024];
    struct timeval t1,t2;
    
    getcontext(&ctx1);
    getcontext(&ctx2);
    ctx1.uc_stack.ss_sp = stack;
    ctx1.uc_stack.ss_size = 1024;
    ctx1.uc_link = &main_ctx;
    ctx2.uc_stack.ss_sp = stack;
    ctx2.uc_stack.ss_size = 1024;
    
    makecontext(&ctx1, test_func1, 0);
    gettimeofday(&t1, NULL);
    swapcontext(&main_ctx, &ctx1);
    gettimeofday(&t2, NULL);
    printf("used time: %ldus\n", (t2.tv_sec - t1.tv_sec)*1000000 + t2.tv_usec - t1.tv_usec); // 10~20us swap twice
    puts("test");
    
    makecontext(&ctx2, (void(*)(void))test_func2, 1, &main_ctx);
    swapcontext(&main_ctx, &ctx2);
    puts("test over");
}

void func1(schedule_t *s, void *args)
{
    puts("11");
    sleep(1);
    coroutine_yield(s);
    int *a = (int *)args;
    printf("%s: %d\n", __func__, a[0]);
}

void func2(schedule_t *s, void *args)
{
    puts("22");
    sleep(2);
    coroutine_yield(s);
    puts("22");
    int *a = (int *)args;
    printf("%s: %d\n", __func__, a[0]);
}

void test_schedule()
{
    int args1[] = {1};
    int args2[] = {2};
    schedule_t *s = schedule_create(2);
    int id1 = coroutine_create(s, func1, args1);
    int id2 = coroutine_create(s, func2, args2);
    printf("id1=%d, id2=%d\n", id1, id2);
    
    struct timeval t1,t2;
    gettimeofday(&t1, NULL);
    
    while (!schedule_finished(s))
    {
        coroutine_resume(s, id2);
        coroutine_resume(s, id1);
    }
    
    gettimeofday(&t2, NULL);
    printf("used time: %ldus\n", (t2.tv_sec - t1.tv_sec)*1000000 + t2.tv_usec - t1.tv_usec); 

    puts("over");
    schedule_destroy(s);
}

void accept_conn(int listenfd, schedule_t *s, coroutine_func handle)
{
    int i=0, connfd = 0;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    
    while (1)
    {
        client_addrlen = sizeof(client_addr);
        connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (connfd > 0)
        {
            int id = coroutine_create(s, handle, &connfd);
            if (id < 0)
            {
                puts("coroutine_create failed");
                close(connfd);
                continue;
            }
            else
            {
                printf("new coroutine %d\n", id);
                coroutine_resume(s, id);
            }
        }
        else 
        {
            // 恢复协程
            for (i=0; i<s->max_index; i++)
            {
                coroutine_resume(s, i);
            }
        }
    }
}

#define MAXLINE 512

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

#define MAX_UTHREAD_SIZE 10

void test_socket()
{
    schedule_t *s = schedule_create(MAX_UTHREAD_SIZE);
    
    int listenfd = start_tcp_server("0.0.0.0", 12345, MAX_UTHREAD_SIZE);
    setnonblocking(listenfd);
    puts("waiting for connection...");
    accept_conn(listenfd, s, handle_conn);
    
    puts("over");
    schedule_destroy(s);
    close(listenfd);
}

void test_nonblock_socket()
{
    int listenfd = start_tcp_server("0.0.0.0", 12345, MAX_UTHREAD_SIZE);
    setnonblocking(listenfd);
    puts("waiting for connection...");
    
    int fds[MAX_UTHREAD_SIZE] = {0};
    int fd_cnt = 0;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    
    char buf[MAXLINE] = {0};
    int connfd, i;
    
    while (1)
    {
        client_addrlen = sizeof(client_addr);
        connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (connfd > 0)
        {
            printf("new connection on fd %d\n", connfd);
            for (i=0; i<fd_cnt; i++)
            {
                if (fds[i] <= 0)
                {
                    fds[i] = connfd;
                    break;
                }
            }
            if (i == fd_cnt)
                fds[fd_cnt++] = connfd;
            
        }
        for (i=0; i<fd_cnt; i++)
        {
            if (fds[i] <= 0)
                continue;
            
            memset(buf, 0, MAXLINE);
            int n = recv(fds[i], buf, MAXLINE-1, MSG_DONTWAIT);
            if (n < 0)
            {
                continue;
            }
            else if (n == 0)
            {
                printf("connection on fd %d closed\n", fds[i]);
                close(fds[i]);
                fds[i] = -1;
                continue;
            }
            else
            {
                buf[n] = '\0';
                printf("connfd: %d, recv: %s\n", fds[i], buf);
                if (strcmp(buf, "exit") == 0)
                    goto out;
                send(fds[i], buf, strlen(buf), MSG_DONTWAIT);
            }
        }
    }
out:
    close(listenfd);
}

int main(int argc, char **argv)
{
    if (argc < 2){
        printf("Usage: %s option\n", argv[0]);
        return -1;
    }
    int op = atoi(argv[1]);
    switch (op)
    {
        case 0:
            test_getcontext();
            break;
        case 1:
            test_swapcontext();
            break;
        case 2:
            test_schedule();
            break;
        case 3:
            test_socket();
            break;
        case 4:
            test_nonblock_socket();
        default:
            printf("unkown option\n");
            return -1;
    }
    return 0;
}
