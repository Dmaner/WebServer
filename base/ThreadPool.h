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

template <typename T>
ThreadPool<T>::ThreadPool(int nthread, int mrequest) : thread_number(nthread), max_request_number(mrequest),
                                                       stop(false), threads(NULL)
{
    if (thread_number <= 0 || max_request_number <= 0)
    {
        error_handle("thread pool init");
    }
    threads = new pthread_t[thread_number];
    if (!threads)
    {
        error_handle("thread pool init");
    }

    for (int i = 0; i < thread_number; i++)
    {
        printf("create the %d thread\n", i);
        if (pthread_create(threads + i, NULL, worker, this) != 0)
        {
            delete[] threads;
            error_handle("thread pool init");
        }
        if (pthread_detach(threads[i]))
        {
            delete[] threads;
            error_handle("thread pool init");
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete[] threads;
    stop = true;
}

template <typename T>
bool ThreadPool<T>::append(T* task)
{
    work_queue_locker.lock();
    if (work_queue.size() > max_request_number)
    {
        work_queue_locker.unlock();
        return false;
    }
    work_queue.push_back(task);
    work_queue_locker.unlock();
    queue_state.post();
    return true;
}

template <typename T>
void *ThreadPool<T>::worker(void *args)
{
    ThreadPool *pool = (ThreadPool *)args;
    pool->run();
    return pool;
}

template <typename T>
void ThreadPool<T>::run()
{
    while (stop)
    {
        queue_state.wait();
        work_queue_locker.lock();
        if (work_queue.empty())
        {
            work_queue_locker.unlock();
            continue;
        }
        T *request = work_queue.front();
        work_queue.pop_front();
        work_queue_locker.unlock();
        if (!request)
        {
            continue;
        }
        // T类型必须实现process方法
        request->process();
    }
}

#endif