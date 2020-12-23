/*
 * @Author: dman 
 * @Date: 2020-12-23 16:50:32 
 * @Last Modified by:   dman 
 * @Last Modified time: 2020-12-23 16:50:32 
 */
#include "Util.h"

void error_handle(const char* msg)
{
    perror(msg);
    exit(1);
}