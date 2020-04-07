//
// Created by jmq on 2020/4/4.
//

#ifndef SOCKET_COMMON_H
#define SOCKET_COMMON_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cerrno>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <algorithm>


//服务器ip
#define SERVER_IP  "192.168.0.139"

//服务器端口号
#define SERVER_PORT   6666

//缓冲区大小
#define BUF_SIZE 2048

// 新客户端收到的欢迎信息
#define SERVER_WELCOME  "Welcome new client join to the chat Room, ID is : #%d"

//epoll 支持的最大句柄数
#define EPOLL_SIZE  5000

// 线程池线程个数
#define THREAD_NUM 4


#define CAUTION "There is only you in the chat room"

#define NO_ONE "#%d is not in the chat room"

#endif //SOCKET_COMMON_H
