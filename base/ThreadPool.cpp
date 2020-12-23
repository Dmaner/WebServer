/*
 * @Author: dman 
 * @Date: 2020-12-23 19:38:01 
 * @Last Modified by: dman
 * @Last Modified time: 2020-12-23 19:38:38
 */
#include "ThreadPool.h"

template <typename T>
ThreadPool<T>::ThreadPool(int nthread, int mrequest) : thread_number(nthread), max_request_number(mrequest),
                                                       stop(false), threads(NULL)
{
    if (thread_number <= 0 || max_request_number <= 0)
    {
        error_handle("thread pool init");
    }
    thrads = new pthread_t[thread_number];
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