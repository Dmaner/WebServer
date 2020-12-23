#ifndef MY_THREADPOOL_H
#define MY_THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "Locker.h"
#include "Sema.h"

template <typename T>
class ThreadPool
{
private:
    // 线程池线程数
    int thread_number;
    // 请求队列最大请求数
    int max_request_number;
    // 存线程的数组
    pthread_t *threads;
    // 请求队列
    std::list<T *> work_queue;
    // 请求队列互斥锁
    Locker work_queue_locker;
    // 判断请求队列是否为空
    sem queue_state;
    // 判断是否结束线程
    bool stop;

    // 工作线程
    static void *worker(void *arg);
    // 查找请求队列是否有任务就绪
    void run();

public:
    ThreadPool(int nthread = 8, int mrequest = 10000);
    ~ThreadPool();

    // 请求队列添加任务
    bool append(T* task);
};

#endif