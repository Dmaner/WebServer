/*
 * @Author: dman 
 * @Date: 2020-12-23 16:28:40 
 * @Last Modified by: dman
 * @Last Modified time: 2020-12-23 18:45:06
 */
#ifndef MY_SEMAPHORE_H
#define MY_SEMAPHORE_H

#include <semaphore.h>

// 信号量
class sem
{
private:
    sem_t m_sem;
public:
    sem();
    ~sem();
    // 等待信号量
    bool wait();
    // 增加信号量
    bool post();
};

#endif