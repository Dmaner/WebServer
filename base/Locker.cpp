#include "Locker.h"

Locker::Locker()
{
    if (pthread_mutex_init(&mutex, NULL) == -1)
    {
        error_handle("locker init");
    }
}

Locker::~Locker()
{
    pthread_mutex_destroy(&mutex);
}

bool Locker::lock()
{
    return pthread_mutex_lock(&mutex) == 0;
}

bool Locker::unlock()
{
    return pthread_mutex_unlock(&mutex) == 0;
}