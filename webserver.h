#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"

const int MAX_FD = 65536;           /* 最大文件描述符 */
const int MAX_EVENT_NUMBER = 10000; /* 最大事件数 */
const int TIMESLOT = 5;             /* 最小超时单位 */

class WebServer
{
public:
    WebServer();
    ~WebServer();

    /* 服务器初始化 */
    void init(int port, string user, string passWord, string databaseName,
              int log_write, int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);

    void thread_pool();                                        /* 初始化线程池 */
    void sql_pool();                                           /* 初始化数据库连接池 */
    void log_write();                                          /* 初始化日志 */
    void trig_mode();                                          /* 初始化触发模式 */
    void eventListen();                                        /* 添加事件监听 */
    void eventLoop();                                          /* 开启事件循环 */
    void timer(int connfd, struct sockaddr_in client_address); /* 初始化client_data */
    void adjust_timer(util_timer *timer);                      /* 修改定时器时间 */
    void deal_timer(util_timer *timer, int sockfd);            /* 关闭用户链接 */
    bool dealclinetdata();                                     /* 处理用户数据 */
    bool dealwithsignal(bool &timeout, bool &stop_server);     /* 处理信号事件 */
    void dealwithread(int sockfd);                             /* 处理读事件 */
    void dealwithwrite(int sockfd);                            /* 处理写事件 */

public:
    //基础
    int m_port;       /* 主机号 */
    char *m_root;     /* 服务器根目录 */
    int m_log_write;  /* 是否持久化日志 */
    int m_close_log;  /* 是否开启日志 */
    int m_actormodel; /* Rreact or Proactor*/

    int m_pipefd[2];  /* 用于父子进程通信 */
    int m_epollfd;    /* epollfd */
    http_conn *users; /* 存用户 */

    //数据库相关
    connection_pool *m_connPool; /* 数据库连接池 */
    string m_user;               /* 登陆数据库用户名 */
    string m_passWord;           /* 登陆数据库密码 */
    string m_databaseName;       /* 使用数据库名 */
    int m_sql_num;               /* 最大连接树 */

    //线程池相关
    threadpool<http_conn> *m_pool; /* 线程池 */
    int m_thread_num;              /* 最大线程池数 */

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;     /* socket关闭方式 */
    int m_TRIGMode;       /* 触发模式 */
    int m_LISTENTrigmode; /* 服务端 LT or ET */
    int m_CONNTrigmode;   /* 客户端LT  */

    //定时器相关
    client_data *users_timer; /* 用户数据 */
    Utils utils;              /* 定时器相关函数 */
};
#endif
