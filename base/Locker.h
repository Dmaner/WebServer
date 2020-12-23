/*
 * @Author: dman 
 * @Date: 2020-12-23 16:29:36 
 * @Last Modified by: dman
 * @Last Modified time: 2020-12-23 17:02:02
 */
#ifndef MY_MUTEX_H
#define MY_MUTEX_H

#include <pthread.h>
#include "Util.h"

class Locker
{
private:
    pthread_mutex_t mutex;
public:
    Locker();
    ~Locker();
    // 获取互斥锁
    bool lock();
    // 释放锁
    bool unlock();
};

#endif