//
// Created by jmq on 2020/4/13.
//

#ifndef SERVER_UTILITY_H
#define SERVER_UTILITY_H
#include "server.h"
#include <string>

struct Message;

shared_ptr<Message> ParseData(int clientFd, std::string data);

#endif //SERVER_UTILITY_H
