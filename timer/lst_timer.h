#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;

/* 保存客户端数据 */
struct client_data
{
    sockaddr_in address; /* soket地址 */
    int sockfd;          /* socket描述符 */
    util_timer *timer;   /* 定时器 */
};

/* 定时器 */
class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;                  /* 超时时间 */
    void (*cb_func)(client_data *); /* 回调函数 */
    client_data *user_data;         /* 用户数据, 做回调函数的参数 */
    util_timer *prev;               /* 前一个定时器 */
    util_timer *next;               /* 后一个定时器 */
};

/* 定时器升序链表实现 */
class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer);    /* 添加定时器 */
    void adjust_timer(util_timer *timer); /* 修改定时器 */
    void del_timer(util_timer *timer);    /* 删除定时器 */
    void tick();                          /* 查看当前时间是否有定时器触发 */

private:
    void add_timer(util_timer *timer, util_timer *lst_head);

    util_timer *head;
    util_timer *tail;
};

class Utils
{
public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;

public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);
};

void cb_func(client_data *user_data);

#endif
