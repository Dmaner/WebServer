#ifndef MY_EPOLL_H
#define MY_EPOLL_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>

int setnonblocking(int fd); 
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);
void removefd(int epollfd, int fd);
void modfd(int epollfd, int fd, int ev, int TRIGMode);

#endif