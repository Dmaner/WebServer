#include "httpconn.h"

/* http状态响应信息 */
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to statisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

/* 网站根目录 */
const char *root_path = "/var";

// 设置文件非阻塞
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/* epoll相关函数 */
void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
///////////////////////////////////////
//           初始化HTTP结构           //
///////////////////////////////////////

int http_conn::user_count = 0;
int http_conn::epollfd = -1;

// 关闭连接
void http_conn::close_conn(bool real_close)
{
    if (real_close && sock_fd != -1)
    {
        removefd(epollfd, sock_fd);
        sock_fd = -1;
        // 关闭连接用户数减一
        user_count--;
    }
}

// 添加链接初始化
void http_conn::init(int sockfd, const sockaddr_in &addr)
{
    sock_fd = sockfd;
    sock_address = addr;
    addfd(epollfd, sock_fd, true);
    user_count++;
    init();
}

// 初始化http_conn结构
void http_conn::init()
{
    curr_state = CHECK_STATE_REQESTLINE;
    keep_alive = false;
    method = GET;
    url = 0;
    host = 0;
    http_version = 0;
    content_length = 0;
    start_line = 0;
    checked_idx = 0;
    read_idx = 0;
    write_idx = 0;
    memset(read_buf, 0, READ_BUFFER_SIZE);
    memset(write_buf, 0, WRITE_BUFFER_SIZE);
    memset(file_path, 0, FILENAME_LEN);
}

/////////////////////////////////
//         解析http请求         //
/////////////////////////////////

/**
 * 获得一行
*/
char *http_conn::get_line()
{
    return read_buf + start_line;
}

/**
 * 从状态机
*/
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for (; checked_idx < read_idx; checked_idx)
    {
        temp = read_buf[checked_idx];
        if (temp == '\r')
        {
            if (checked_idx + 1 == read_idx)
            {
                return LINE_OPEN;
            }
            else if (read_buf[checked_idx + 1] == '\n')
            {
                read_buf[checked_idx++] = 0;
                read_buf[checked_idx++] = 0;
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (checked_idx > 1 && read_buf[checked_idx - 1] == '\r')
            {
                read_buf[checked_idx - 1] = 0;
                read_buf[checked_idx++] = 0;
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/**
 * 无阻塞读
*/
bool http_conn::read()
{
    if (read_idx >= READ_BUFFER_SIZE)
    {
        return false;
    }
    int ret = 0;
    while (true)
    {
        ret = recv(sock_fd, read_buf + read_idx, READ_BUFFER_SIZE - read_idx, 0);
        if (ret == -1)
        {
            // 无数据可读则先返回
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return false;
        }
        else if (ret == 0)
        {
            return false;
        }
        read_idx += ret;
    }
    return true;
}

/**
 *  解析http请求行, 获得请求方法, url, 版本号
 */
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    /* 获取请求资源位置 */
    url = strpbrk(text, " \t");
    if (!url)
    {
        return BAD_REQUEST;
    }
    *url++ = 0;
    url += strspn(url, " \t");

    /* 获取http请求方法 */
    if (strcasecmp(text, "GET") == 0)
    {
        method = GET;
    }
    else
    {
        return BAD_REQUEST;
    }

    /* 获得http版本信息 */
    http_version = strpbrk(url, " \t");
    if (!http_version)
    {
        return BAD_REQUEST;
    }
    // 在url后添加终止符
    *http_version = 0;
    // 跳过空白, 让http_version指向版本
    http_version += strspn(http_version, " \t");
    if (strcasecmp(http_version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }

    /* 删除http://前缀 */
    if (strncasecmp(url, "http://", 7) == 0)
    {
        url += 7;
        url = strchr(url, '/');
    }
    if (!url || url[0] != '/')
    {
        return BAD_REQUEST;
    }

    curr_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

/**
 * 解析http请求的一个头部信息
*/
http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    /* 如果头部解析完成, 判断是否还要解析消息体 */
    if (text[0] == '\0')
    {
        if (content_length != 0)
        {
            // 状态机转移到CHECK_STATE_CONTENT状态
            curr_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // 无消息体则返回
        return GET_REQUEST;
    }
    /* 处理Connection头部字段 */
    else if (strncasecmp(text, "Connnection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            keep_alive = true;
        }
    }
    /* 处理Content-Length头部字节 */
    else if (strncasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        content_length = atol(text);
    }
    /* 处理Host头部字段 */
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        host = text;
    }
    else
    {
        printf("oop! unknow header %s\n", text);
    }

    return NO_REQUEST;
}

/** 
 * 我们没有真正解析HTTP请求的消息体，
 * 只是判断它是否被完整地读入了
 */
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    if (read_idx >= (content_length + checked_idx))
    {
        text[content_length] = '\0';
        return GET_REQUEST;
    }

    return NO_REQUEST;
}

/**
 * 主状态机
*/

http_conn::HTTP_CODE http_conn::process_request()
{
    // 当前行读取状态
    LINE_STATUS line_status = LINE_OK;
    // HTTP处理结果
    HTTP_CODE ret = NO_REQUEST;

    char *text = 0;

    while (((curr_state == CHECK_STATE_CONTENT) &&
            line_status == LINE_OK) ||
           (line_status = parse_line()) == LINE_OK)
    {
        text = get_line();
        start_line = checked_idx;
        fprintf(stdout, "got 1 http line: %s\n", text);

        // 状态转换
        switch (curr_state)
        {
        // 分析请求行
        case CHECK_STATE_REQESTLINE:
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        //分析头部字段
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if (ret == GET_REQUEST)
            {
                return do_request();
            }
            break;
        }
        // 分析消息体
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
            {
                return do_request();
            }
            line_status = LINE_OPEN;
            break;
        }
        default:
        {
            return INTERNAL_ERROR;
        }
        }
    }
    return NO_REQUEST;
}

/**
 * 完成http请求报文分析 
*/
http_conn::HTTP_CODE http_conn::do_request()
{
    strcpy(file_path, root_path);
    int len = strlen(root_path);
    strncpy(file_path + len, url, FILENAME_LEN - len - 1);
    // 无文件
    if (stat(file_path, &file_stat) < 0)
    {
        return NO_RESOURCE;
    }
    // 无权限
    if (!(file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUEST;
    }
    // 是目录
    if (S_ISDIR(file_stat.st_mode))
    {
        return BAD_REQUEST;
    }

    int fd = open(file_path, O_RDONLY);
    // TODO: check
    file_address = (char *)mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

/**
 * 解除文件对内存的映射
*/

void http_conn::unmap()
{
    if (file_address)
    {
        munmap(file_address, file_stat.st_size);
        file_address = 0;
    }
}

////////////////////////////////////
//          构造http响应           //
////////////////////////////////////

/**
 * 无阻塞写
*/
bool http_conn::write()
{
    int ret = 0;
    int bytes_have_send = 0;
    int bytes_to_send = write_idx;
    // 没有要数据要发出
    if (bytes_to_send == 0)
    {
        modfd(epollfd, sock_fd, EPOLLIN);
        init();
        return true;
    }

    while (true)
    {
        ret = writev(sock_fd, iv, iv_conut);
        if (ret <= -1)
        {
            // TCP写空间不足
            if (errno == EAGAIN)
            {
                modfd(epollfd, sock_fd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        bytes_to_send -= ret;
        bytes_have_send += ret;
        // TODO:check
        if (bytes_to_send <= bytes_have_send)
        {
            unmap();
            if (keep_alive)
            {
                init();
                modfd(epollfd, sock_fd, EPOLLIN);
                return true;
            }
            else
            {
                modfd(epollfd, sock_fd, EPOLLIN);
                return false;
            }
        }
    }
}

/**
 * 往写缓冲区中写入待发送数据
*/
bool http_conn::add_response(const char *format, ...)
{
    if (write_idx >= WRITE_BUFFER_SIZE)
    {
        return false;
    }

    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(write_buf + write_idx, WRITE_BUFFER_SIZE - 1 - write_idx,
                       format, arg_list);
    if (len >= WRITE_BUFFER_SIZE - 1 - write_idx)
    {
        return false;
    }
    write_idx += len;
    va_end(arg_list);
    return true;
}

bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len)
{
    add_content_length(content_len);
    add_linger();
    add_blank_line();
    return true;
}

bool http_conn::add_linger()
{
    return add_response("Connection: %s\r\n", (keep_alive == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line()
{
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char *content)
{
    return add_response("%s", content);
}

/**
 * 根据解析请求返回相应http响应
*/
bool http_conn::process_response(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500, error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
        {
            return false;
        }
        break;
    }
    case BAD_REQUEST:
    {
        add_status_line(400, error_400_title);
        add_headers(strlen(error_400_form));
        if (!add_content(error_400_form))
        {
            return false;
        }
        break;
    }
    case NO_RESOURCE:
    {
        add_status_line(404, error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
        {
            return false;
        }
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_status_line(403, error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
        {
            return false;
        }
        break;
    }
    case FILE_REQUEST:
    {
        add_status_line(200, ok_200_title);
        if (file_stat.st_size != 0)
        {
            add_headers(file_stat.st_size);
            iv[0].iov_base = write_buf;
            iv[0].iov_len = write_idx;
            iv[0].iov_base = file_address;
            iv_conut = 2;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
            {
                return false;
            }
        }
    }
    default:
    {
        return false;
    }
    }
    iv[0].iov_base = write_buf;
    iv[0].iov_len = write_idx;
    iv_conut = 1;
    return true;
}

/**
 * 主要线程
*/
void http_conn::process()
{
    HTTP_CODE request_ret = process_request();
    if (request_ret == NO_REQUEST)
    {
        modfd(epollfd, sock_fd, EPOLLIN);
        return;
    }
    bool response_ret = process_response(request_ret);
    if (!response_ret)
    {
        close_conn();
    }
    modfd(epollfd, sock_fd, EPOLLOUT);
}