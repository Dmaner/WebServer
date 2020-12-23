/*
 * @Author: dman 
 * @Date: 2020-12-23 16:30:51 
 * @Last Modified by: dman
 * @Last Modified time: 2020-12-23 16:58:31
 */
#include "Sema.h"
#include "Util.h"

sem::sem()
{
    // 信号量进程共享
    int pshared = 0;
    // 信号量初始值
    unsigned int value = 0;
    // 初始化信号量
    if (sem_init(&m_sem, pshared, value) == -1)
    {
        error_handle("sem init");
    }
}

sem::~sem()
{
    if (sem_destroy(&m_sem) == -1)
    {
        error_handle("sem destroy");
    }
}

bool sem::wait()
{
    return sem_wait(&m_sem) == 0;
}

bool sem::post()
{
    return sem_post(&m_sem) == 0;
}