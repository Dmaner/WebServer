#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdarg.h>

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    // HTTP 方法
    enum MEHTOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };

    // HTTP状态码
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

    // 主状态机的状态
    enum CHECK_STATE
    {
        CHECK_STATE_REQESTLINE = 0, /* 分析请求行 */
        CHECK_STATE_HEADER,         /* 分析HTTP头部 */
        CHECK_STATE_CONTENT         /* 分析HTTP报文内容 */
    };

    // 从状态机状态
    enum LINE_STATUS
    {
        LINE_OK = 0, /* 完整读取一行 */
        LINE_BAD,    /* 行读取失败 */
        LINE_OPEN    /* 行数据仍在读取 */
    };

private:
    /* 绑定的套接字描述符和及其地址 */
    int sock_fd;
    sockaddr_in sock_address;

    /* 读写缓冲区 */
    char read_buf[READ_BUFFER_SIZE];
    char write_buf[WRITE_BUFFER_SIZE];

    int read_idx;    /* 读入字节的下一个位置 */
    int checked_idx; /* 当前正在读取位置 */
    int start_line;  /* 正在解析位置 */
    int write_idx;   /* 写缓冲区待发送的字节数 */

    CHECK_STATE curr_state;       /* 主状态机状态 */
    MEHTOD method;                /* 请求方法 */
    char file_path[FILENAME_LEN]; /* 请求文件完整路径 */
    char *url;                    /* 请求文件名 */

    char *http_version; /* HTTP版本号 */
    char *host;         /* 主机名 */
    int content_length; /* 请求报文长度 */
    bool keep_alive;    /* 是否保持连接 */

    char *file_address;    /* 请求文件被mmap位置 */
    struct stat file_stat; /* 保存请求文件属性 */

    /* 用于writev和readv */
    struct iovec iv[2];
    int iv_conut;

public:
    static int epollfd;
    static int user_count;

    http_conn();
    ~http_conn();

public:
    // 初始化连接
    void init(int sockfd, const sockaddr_in &addr);
    // 关闭连接
    void close_conn(bool real_clos = true);
    // 处理http报文
    void process();
    // 无阻塞读
    bool read();
    // 无阻塞写
    bool write();

private:
    void init();
    // 处理HTTP请求
    HTTP_CODE process_request();
    // 构建HTTP响应
    bool process_response();

    /* 处理http请求报文 */
    HTTP_CODE parse_request_line(char *text);
    // 解析报文头
    HTTP_CODE parse_headers(char *text);
    // 解析报文主体
    HTTP_CODE parse_content(char *text);
    // 发出请求
    HTTP_CODE do_request();
    // 获取一行
    char *get_line();
    // 获取行读取状态
    LINE_STATUS parse_line();

    /* 处理http相应报文 */
    void unmap();
    // 添加响应
    bool add_response(const char *format, ...);
    // 添加响应报文主体
    bool add_content(const char *content);
    // 添加http状态码
    bool add_status_line(int status, const char *title);
    // 添加响应头
    bool add_headers(int content_length);
    // 添加响应主体长度
    bool add_content_length(int content_length);
    // 添加keep alive信息
    bool add_linger();
    // 添加空白行
    bool add_blank_line();
};

#endif