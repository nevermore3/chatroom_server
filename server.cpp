#include "server.h"
#include <iostream>
#include <sstream>
#include <mutex>
#include "thread.h"
using namespace std;

Server::Server() {
    cout<<"初始化服务器sockAddr信息"<<endl;
    serverAddr_.sin_family = PF_INET;
    serverAddr_.sin_port = htons(SERVER_PORT);
    serverAddr_.sin_addr.s_addr = inet_addr(SERVER_IP);

    listenFd_ = 0;
    epFd_ = 0;
}

void Server::Init() {
    cout<<"Init"<<endl;

    cout << "Main Thread : " << this_thread::get_id() << "  Start" << endl;

    run_ = true;

    // 线程池初始化
    threadPool_ = new ThreadPool(THREAD_NUM);

    listenFd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        perror("Create socket Error");
        exit(-1);
    }

    //bind
    if (bind(listenFd_, (struct sockaddr *)&serverAddr_, sizeof(serverAddr_)) < 0) {
        perror("Bind Error");
        exit(-1);
    }

    //listen
    if (listen(listenFd_, 5) < 0) {
        perror("Listen Error");
        exit(-1);
    }

    //创建epoll句柄
    epFd_ = epoll_create(EPOLL_SIZE);
    if (epFd_ < 0) {
        perror("epoll_create Error");
        exit( -1);
    }

    //向epoll中添加监听事件
    AddFd(epFd_, listenFd_);

}

void Server::Close() {
    close(listenFd_);
    close(epFd_);
}
/*
 * enable : 决定epoll是LT或者ET模式
 */
void Server::AddFd(int epFd, int fd) {
    struct epoll_event  ev;
    ev.data.fd = fd;
    // true : 边缘模式， 一次读不完下次不在读直到有消息再来

    bool enable = true;
    ev.events = EPOLLIN;
    if (enable) {
        ev.events = EPOLLIN | EPOLLET;
    }
    if (epoll_ctl(epFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl Error");
        exit(-1);
    }

    //设置socket为非阻塞
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    cout<<fd<<" 加入到 EPOLL中"<<endl;
}

void Server::Start() {
    //epoll 事件队列
    static struct epoll_event events[EPOLL_SIZE];

    Init();

    while(run_) {
        // 若没有事件则一直阻塞
        int readyNum = epoll_wait(epFd_, events, EPOLL_SIZE, -1);

        if (readyNum < 0) {
            perror("Epoll_wait error");
            break;
        }

        // 处理事件
        cout << "就绪事件个数为: " << readyNum << endl;
        for (int i = 0; i < readyNum; i++) {
            // 异常处理
            if ((events[i].events & EPOLLERR) ||(events[i].events & EPOLLHUP) || (!(events[i].events &EPOLLIN))) {
                    cout << "epoll error" << endl;
                    close(events[i].data.fd);
                    continue;
            }

            int sockFd = events[i].data.fd;
            stringstream clientInfo;

            if (sockFd == listenFd_) {
                struct sockaddr_in clientAddress;
                socklen_t len = sizeof(struct sockaddr_in);
                int clientFd = accept(listenFd_, (struct sockaddr*) &clientAddress, &len);
                clientInfo << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port);
                cout << "New Client Connect. IP :" << clientInfo.str() << endl;

                // 将新建立连接的socket加入epoll中，进行监控
                AddFd(epFd_, clientFd);

                clientList_.push_back(clientInfo.str());
                cout<<"Client Size :" <<clientList_.size()<<endl;
                SendWelcome(clientFd);
            } else {

                struct sockaddr clientAddress;
                socklen_t  len = sizeof(sockaddr);
                if (getpeername(sockFd, &clientAddress, &len) != 0) {
                    perror("getpeername error");
                    exit(-1);
                }
                struct sockaddr_in *address = (struct sockaddr_in *) &clientAddress;

                // 消息事件, 处理sockFd传送过来的消息，并发给其客户端

                // 多线程
                function<int(int, struct sockaddr_in&)> processData = bind(&Server::Process, this, placeholders::_1, placeholders::_2);
                threadPool_->AddTask(processData, sockFd, ref(*address));

                //单线程
//                if (Process(sockFd, *address) < 0) {
//                    clientInfo << inet_ntoa((*address).sin_addr)<<":"<<ntohs((*address).sin_port);
//                    RemoveClient(clientInfo.str());
//                }
            }
        }
    }
}

void Server::SendWelcome(int clientFd) {
    char message[BUF_SIZE];
    bzero(message, BUF_SIZE);
    sprintf(message, SERVER_WELCOME, clientFd);
    if (send(clientFd, message, strlen(message), 0) < 0) {
        perror("send welcome error");
        exit(-1);
    }
}

int Server::Process(int clientFd, struct sockaddr_in &address) {
    /*
     *  打印当前线程信息
     */
    cout << "Thread : " << this_thread::get_id() << "  Work" << endl;

    int flag = 1;  // -1: clientFd 掉线  1: 正常
    stringstream clientInfo;
    clientInfo << "Client:" << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port);

    char recvBuf[BUF_SIZE] = {0};
    char sendBuf[BUF_SIZE] = {0};

    do {
        bzero(sendBuf, BUF_SIZE);
        bzero(recvBuf, BUF_SIZE);
        int len = recv(clientFd, recvBuf, BUF_SIZE, 0);

        if (len == 0) {
            // 接受缓冲区接受数据时客户端断开链接 client 调用close(socket)
            cout << clientInfo.str() << "    断开链接" << endl;
            flag = -1;
            break;
        } else if (len == -1) {
            /*
             *  两种情况 1、发送数据完毕 2、异常
             */
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                cout<<"-------------------"<<endl;
            } else {
                perror("Recv error");
                flag = -1;
            }
            break;
        } else {
            /*
             *  1.接受client发送过来的数据
             *  2.给client回复信息
             */
            if (!strcmp(recvBuf, "exit")) {
                cout << clientInfo.str() << " 打出exit, 并中断了与服务器的链接" << endl;
                flag = -1;
            } else {
                cout << clientInfo.str() << " : " << string(recvBuf) << endl;

                // 给客户端发送确认数据
                sprintf(sendBuf, "Server Receive : %s .", recvBuf);
                if (send(clientFd, sendBuf, strlen(sendBuf), 0) < 0) {
                    perror("server send error");
                    exit(-1);
                }
            }
        }
    } while (run_);  //接受缓冲区消息大小大于BUF_SIZE，循环多次接受


    // 在多线程内处理clientList_
    if (flag == -1) {
        stringstream removeInfo;
        removeInfo << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port);
        RemoveClient(removeInfo.str());
    }

    // 等待5s,观察多线程机制
    this_thread::sleep_for(chrono::seconds(5));

    return flag;
}

Server::~Server() {
    Close();
}

void Server::RemoveClient(string clientInfo) {
    unique_lock<mutex>clientListLock (clientListMutex_);
    clientList_.remove(clientInfo);
    cout<< clientInfo << " 被移除Client List, 当前size " << clientList_.size() << endl;
}

int Server::SendMessageToAll(int fromId, string &msg) {

    char sendBuf[BUF_SIZE] = {0};
    bzero(sendBuf, BUF_SIZE);

    if (chatRoom_.size() == 1) {
        if (send(fromId, CAUTION, strlen(CAUTION), 0) < 0) {
            perror("send message to all send error");
            exit(-1);
        }
        return 1;
    }
    // 向聊天室的所有人发送信息，包括自己
    sprintf(sendBuf, "client %d say : %s", fromId, msg.c_str());

    for (auto clientId : chatRoom_) {
        if (send(clientId, sendBuf, strlen(sendBuf), 0) < 0) {
            perror("send message to all send error");
        }
    }
    return 1;
}

int Server::SendMessageToOne(int fromId, int toId, string &msg) {
    char sendBuf[BUF_SIZE] = {0};
    bzero(sendBuf, BUF_SIZE);

    if (find(chatRoom_.begin(), chatRoom_.end(), toId) == chatRoom_.end()) {
        sprintf(sendBuf, NO_ONE, toId);
        if (send(fromId, sendBuf, strlen(sendBuf), 0) < 0) {
            perror("send message to one send error");
            exit(-1);
        }
        return 1;
    }

    sprintf(sendBuf, "client %d say : %s", fromId, msg.c_str());
    if (send(toId, sendBuf, strlen(sendBuf), 0) < 0) {
        perror("send message to one send error");
        exit(-1);
    }
    return 1;
}

