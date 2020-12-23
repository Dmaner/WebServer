#include "Condition.h"

Condition::Condition()
{
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        error_handle("condition init");
    }
    if (pthread_cond_init(&cond, NULL) != 0)
    {
        pthread_mutex_destroy(&mutex);
        error_handle("conditon init");
    }
}

Condition::~Condition()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

bool Condition::wait()
{
    pthread_mutex_lock(&mutex);
    int ret = pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
    return ret == 0;
}

bool Condition::signal()
{
    return pthread_cond_signal(&cond);
}