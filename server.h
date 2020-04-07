//
// Created by jmq on 2020/4/4.
//

#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include "common.h"
#include "thread.h"
using namespace std;

class Server {
public:
    Server();

    /*
     * 1. create Listen socket
     * 2. bind server address & port
     * 3. listen port
     * 4. crate epoll
     * 5. add listen socket to epoll
     */
    void Init();

    void Start();

    void Close();

    ~Server();

private:
    void RemoveClient(string clientInfo);

    void AddFd(int epFd, int fd);

    //给客户端发送欢迎消息
    static void SendWelcome(int clientFd);

    int Process(int clientFd, struct sockaddr_in &address);

private:
    //服务器地址serverAddr信息
    struct sockaddr_in serverAddr_;

    // 监听的socket
    int listenFd_;

    //epoll句柄
    int epFd_;

    // 客户端列表锁
    mutex clientListMutex_;

    //客户端列表 IP:Port 认定一个client
    list<string>clientList_;

    bool stop_;

    // ThreadPool
    ThreadPool *threadPool_;
};

// 服务器和客户端间传送数据的封装
struct Message {
    int type_;
    int fromID_;
    int toID_;
    char content_[BUF_SIZE];
};

#endif //SOCKET_SERVER_H
