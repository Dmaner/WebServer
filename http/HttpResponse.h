#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_conn.h"

class HttpResponse
{
private:
    char buffer[http_conn::WRITE_BUFFER_SIZE];
};

#endif 