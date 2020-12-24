#include <signal.h>
#include "net/http/httpconn.h"
#include "base/ThreadPool.h"

#define MAX_FD 65535
#define MAX_EVEN_NUMBER 10000

extern int addfd(int epollfd, int fd, bool one_shot);
extern int removefd(int epollfd, int fd);

typedef void (*SigFunc)(int);

void addsig(int sig, SigFunc handler, bool restart = true)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    if (sigaction(sig, &sa, NULL) == -1)
    {
        error_handle("sigaction");
    }
}

void show_error(int connfd, const char *info)
{
    fprintf(stderr, "%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        fprintf(stdout, "usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    addsig(SIGPIPE, SIG_IGN);

    ThreadPool<http_conn> *pool = NULL;
    try
    {
        pool = new ThreadPool<http_conn>;
    }
    catch (...)
    {
        return 1;
    }
    http_conn *users = new http_conn[MAX_FD];
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct linger tmp = {1, 0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));

    ret = listen(listenfd, 5);

    epoll_event events[MAX_EVEN_NUMBER];
    int epollfd = epoll_create1(0);
    addfd(epollfd, listenfd, false);
    http_conn::epollfd = epollfd;

    while (true)
    {
        int number = epoll_wait(epollfd, events, MAX_EVEN_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            fprintf(stderr, "Epoll failure\n");
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addr_len = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addr_len);
                if (connfd < 0)
                {
                    fprintf(stderr, "errno is: %d", errno);
                    continue;
                }
                if (http_conn::user_count >= MAX_FD)
                {
                    show_error(connfd, "Internal server busy");
                    continue;
                }

                users[connfd].init(connfd, client_address);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();
            }
            else if (events[i].events & EPOLLIN)
            {
                if (users[sockfd].read())
                {
                    pool->append(users + sockfd);
                }
                else
                {
                    users[sockfd].close_conn();
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
        }
    }
    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;
    return 0;
}
