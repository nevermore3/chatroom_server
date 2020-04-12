#include "server.h"
#include <iostream>
#include <sstream>
#include "thread.h"
using namespace std;

Server::Server() {
    cout << "初始化服务器sockAddr信息" << endl;
    serverAddr_.sin_family = PF_INET;
    serverAddr_.sin_port = htons(SERVER_PORT);
    serverAddr_.sin_addr.s_addr = inet_addr(SERVER_IP);

    listenFd_ = 0;
    epFd_ = 0;

    // 初始化数据库
    db_ = make_shared<DataBase>();
    db_->InitDB();

}

void Server::Init() {
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


    // 注册命令初始化


    cout<<" Server Init"<<endl;
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
                    //cout << "epoll error" << endl;
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
                chatRoom_.push_back(clientFd);

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

            // 需要改造，否则命令越来越多的时候 不好维护
            // 使用观察者模式

            if (!strcmp(recvBuf, "exit")) {
                cout << clientInfo.str() << " 打出exit, 并中断了与服务器的链接" << endl;
                flag = -1;
            } else if (!strcmp(recvBuf, "who")) {
                CurrentClient(clientFd);
            } else {
                cout << clientInfo.str() << " : " << string(recvBuf) << endl;
                /*
                 *1. 解析客户端发过来的数据
                 * 对 recvbuf进行切分，找出all或者目标fd
                 */
                pair<int, string>msg = ParseData(string(recvBuf));
                // 将消息插入mysql中
                db_->Insert(clientFd, msg.first, msg.second, const_cast<char*>(TABLE_NAME));
                if (msg.first == -1) {
                    SendMessageToAll(clientFd, msg.second);
                } else {
                    SendMessageToOne(clientFd, msg.first, msg.second);
                }

            }
        }
    } while (run_);  //接受缓冲区消息大小大于BUF_SIZE，循环多次接受


    // 在多线程内处理clientList_
    if (flag == -1) {
        stringstream removeInfo;
        removeInfo << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port);
        RemoveClient(clientFd, removeInfo.str());
    }

    // 等待5s,观察多线程机制
    this_thread::sleep_for(chrono::seconds(1));

    return flag;
}

Server::~Server() {
    Close();
}

void Server::RemoveClient(int clientFd, string clientInfo) {
    {
        unique_lock<mutex>clientListLock (clientListMutex_);
        clientList_.remove(clientInfo);
        chatRoom_.remove(clientFd);
        cout<< clientInfo << " 被移除Client List, 当前size " << clientList_.size() << endl;
    }

    // 向chatroom中的所有客户端发消息通知
    if (!chatRoom_.empty()) {
        char sendBuf[BUF_SIZE] = {0};
        bzero(sendBuf, BUF_SIZE);
        sprintf(sendBuf, LEAVE, clientFd);
        string msg(sendBuf);
        SendMessageToAll(clientFd, msg);
    }

}

void Server::SendWelcome(int clientFd) {
    /*
     *  向聊天室的所有人发送welcome消息，以便让其他client知道该客户端的fd
     */
    char message[BUF_SIZE];
    bzero(message, BUF_SIZE);
    sprintf(message, SERVER_WELCOME, clientFd);
    for (auto fd : chatRoom_) {
        if (send(fd, message, strlen(message), 0) < 0) {
            perror("send welcome error");
            exit(-1);
        }
    }
    CurrentClient(clientFd);
}

int Server::SendMessageToAll(int fromId, string &msg) {

    char sendBuf[BUF_SIZE] = {0};
    bzero(sendBuf, BUF_SIZE);
    // 向聊天室的所有人发送信息，包括自己
    sprintf(sendBuf, "client %d say : %s", fromId, msg.c_str());

    for (auto clientId : chatRoom_) {
        if (send(clientId, sendBuf, strlen(sendBuf), 0) < 0) {
            perror("send message to all send error");
            exit(-1);
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

void Server::CurrentClient(int clientFd) {
    // 向客户端发送当前在chat room的所有fd
    bool flag = true;
    string info = "当前聊天室有 : ";
    for (auto fd : chatRoom_) {
        if (fd == clientFd) {
            continue;
        }
        flag = false;
        info += to_string(fd);
        info += "\t";
    }
    if (flag) {
        info += " 就你自己 ：";
        info += to_string(clientFd);
    }
    if (send(clientFd, info.c_str(), info.size(), 0) < 0) {
        perror("send welcome error");
        exit(-1);
    }
}

pair<int, string> ParseData(string data) {
    pair<int, string>ret;
    if (data[0] != '\\') {
        ret.first = -1;
        ret.second = data;
        return ret;
    }

    data.erase(0,1);
    const char* delims = " \t";
    vector<string>tokens;
    int len = data.size();
    char *tok;
    char *line = (char*)malloc(len + 1);
    memset(line, 0, len + 1);
    strcpy(line, data.c_str());

    tok = strtok(line, delims);
    while (tok != nullptr)
    {
        tokens.emplace_back(tok);
        tok = strtok(nullptr, delims);
    }
    string message;
    for (unsigned i = 1; i < tokens.size(); i++) {
        message += tokens[i];
        if (i != tokens.size() - 1) {
            message += " ";
        }
    }
    ret.second = message;
    if (tokens[0] == "ALL" || tokens[0] == "all") {
        ret.first = -1;
    } else {
        ret.first = atoi(tokens[0].c_str());
    }
    return ret;
}
