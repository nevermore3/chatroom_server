//
// Created by jmq on 2020/4/4.
//

#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include "common.h"
#include "thread.h"
#include "database.h"
#include "command.h"
#include "cache.h"
using namespace std;

class CommandManager;

class Server {
public:
    static Server *GetInstance() {
        static Server instance_;
        return &instance_;
    }

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

    inline list<int> &GetChatRoom() {
        return chatRoom_;
    }

    inline shared_ptr<DataBase> &GetDB() {
        return db_;
    }

    inline Cache *GetCache() {
        return cache_;
    }
    ~Server();

    int SendMessageToOne(int fromId, int toId, char *msg);

    int SendMessageToAll(int fromId, char *msg);

private:
    void RemoveClient(int clientFd, string clientInfo);

    void AddFd(int epFd, int fd);

    //给客户端发送欢迎消息
    void SendWelcome(int clientFd);

    int Process(int clientFd, struct sockaddr_in &address);

    // 命令注册
    void RegisterCommand();

    // 插入缓存


private:
    //服务器地址serverAddr信息
    struct sockaddr_in serverAddr_{};

    // 监听的socket
    int listenFd_;

    //epoll句柄
    int epFd_;

    // 客户端列表锁
    mutex clientListMutex_;

    //客户端列表 IP:Port 认定一个client
    list<string>clientList_;

    // 聊天室的client所对应的fd,list 插入删除方便
    list<int>chatRoom_;

    bool run_;

    // ThreadPool
    ThreadPool *threadPool_;

    // database
    shared_ptr<DataBase> db_;

    // redis
    Cache  *cache_;

    //命令管理
    shared_ptr<CommandManager> manager_;

};


#endif //SOCKET_SERVER_H
