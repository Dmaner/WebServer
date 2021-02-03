#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
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
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    // HTTP 方法
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    /* 主状态机的状态 */
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0, /* 分析请求行 */
        CHECK_STATE_HEADER,          /* 分析HTTP头部 */
        CHECK_STATE_CONTENT          /* 分析HTTP报文内容 */
    };

    enum HTTP_CODE
    {
        NO_REQUEST,        /* 请求不完整 */
        GET_REQUEST,       /* 获得完整请求 */
        BAD_REQUEST,       /* 请求报文语法错误 */
        NO_RESOURCE,       /* 没有请求资源 */
        FORBIDDEN_REQUEST, /* 客户权限不够 */
        FILE_REQUEST,      /* 文件请求 */
        INTERNAL_ERROR,    /* 服务器内部错误 */
        CLOSED_CONNECTION  /*服务器关闭连接 */
    };

    /* 从状态机状态 */
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void init(int sockfd, const sockaddr_in &addr,
              char *, int, int, string user,
              string passwd, string sqlname);         /* 初始化连接 */
    void close_conn(bool real_close = true);          /* 关闭连接 */
    void process();                                   /* 总体逻辑 */
    bool read_once();                                 /* 读取用户请求数据 */
    bool write();                                     /* 发送响应数据 */
    void initmysql_result(connection_pool *connPool); /* 初始化数据库状态 */
    int timer_flag;                                   /* 是否开启定时器 */
    int improv;

private:
    char *get_line() { return m_read_buf + m_start_line; };
    void init();                                         /* 初始化成员以及主状态机状态 */
    HTTP_CODE process_read();                            /* 主状态读入一行 */
    bool process_write(HTTP_CODE ret);                   /* HTTP状态返回相应报文 */
    HTTP_CODE parse_request_line(char *text);            /* 解析请求行 */
    HTTP_CODE parse_headers(char *text);                 /* 解析请求头部 */
    HTTP_CODE parse_content(char *text);                 /* 解析请求数据 */
    HTTP_CODE do_request();                              /* 根据请求返回html */
    LINE_STATUS parse_line();                            /* 从状态机读入一行 */
    void unmap();                                        /* 解除映射 */
    bool add_response(const char *format, ...);          /* 添加信息到响应报文 */
    bool add_content(const char *content);               /* 添加响应信息到响应报文 */
    bool add_status_line(int status, const char *title); /* 添加状态码 */
    bool add_headers(int content_length);                /* 添加响应头 */
    bool add_content_type();                             /* 添加响应头部字段Content-Type */
    bool add_content_length(int content_length);         /* 添加响应头部字段Content-Length */
    bool add_linger();                                   /* 添加响应头部字段Connection: */
    bool add_blank_line();                               /* 添加空行到响应头部字段 */

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state; //读为0, 写为1

private:
    /* 绑定的套接字描述符和及其地址 */
    int m_sockfd;
    sockaddr_in m_address;

    /* 读写缓冲区 */
    char m_read_buf[READ_BUFFER_SIZE];
    char m_write_buf[WRITE_BUFFER_SIZE];

    int m_read_idx;    /* 读入字节的下一个位置 */
    int m_checked_idx; /* 当前正在读取位置 */
    int m_start_line;  /* 正在解析位置 */
    int m_write_idx;   /* 写缓冲区待发送的字节数 */

    CHECK_STATE m_check_state;      /* 主状态机状态 */
    METHOD m_method;                /* 请求方法 */
    char m_real_file[FILENAME_LEN]; /* 请求文件完整路径 */
    char *m_url;                    /* 资源定位符 */
    char *m_version;                /* HTTP协议版本 */
    char *m_host;                   /* 主机号 */
    int m_content_length;           /* 报文长度 */
    bool m_linger;                  /* keep-alive标志 */
    char *m_file_address;           /* 主机文件地址 */
    struct stat m_file_stat;        /* 服务器文件属性 */

    /* 用于readv和writev */
    struct iovec m_iv[2];
    int m_iv_count;

    int cgi;             /* 是否启用POST */
    char *m_string;      /* 存储请求头数据 */
    int bytes_to_send;   /* 剩余需要发送的数据 */
    int bytes_have_send; /* 已经发送的数据 */
    char *doc_root;      /* 服务器根目录 */

    map<string, string> m_users; /* 数据库用户 */
    int m_TRIGMode;              /* 触发模式 */
    int m_close_log;             /* 是否开启日志 */

    char sql_user[100];   /* mysql用户 */
    char sql_passwd[100]; /* mysql密码 */
    char sql_name[100];   /* mysql数据库名 */
};

#endif
