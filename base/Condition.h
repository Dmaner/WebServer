/*
 * @Author: dman 
 * @Date: 2020-12-23 17:05:16 
 * @Last Modified by: dman
 * @Last Modified time: 2020-12-23 18:38:22
 */
#ifndef MY_CONDITION_H
#define MY_CONDITION_H

#include <pthread.h>
#include "Util.h"

class Condition
{
private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
public:
    Condition();
    ~Condition();
    // 等待条件变量
    bool wait();
    bool signal();
};

#endif