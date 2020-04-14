//
// Created by jmq on 2020/4/13.
//

#ifndef SERVER_COMMAND_H
#define SERVER_COMMAND_H
#include "server.h"
#include <memory>
#include "common.h"
#include <map>

using namespace std;
// 观察着模式，用于处理客户端发送过来的命令
class Server;
class Message;

shared_ptr<Message> ParseData(int clientFd, string data);

class Command {
public:
    virtual ~Command() = default;

    virtual bool Execute(const shared_ptr<Message>& msg, Server *server) = 0;

    virtual string GetName() = 0;

};


class CommandManager {
public:
    CommandManager() = default;

    void Register(const string& name, const shared_ptr<Command>& command);

    void Register(const shared_ptr<Command>& command);

    void RemoveCommand(string name);

    bool Execute(const shared_ptr<Message>& msg, Server *server);

private:
    // 所有注册的命令
    vector<shared_ptr<Command>> commands_;

    map<string, shared_ptr<Command>> nameCommands_;
};


/*
 *  命令集合
 */

// exit
class Exit : public Command {
public:
    string GetName() override;

    bool Execute(const shared_ptr<Message> &msg, Server *server) override;

private:
    string name_ = "exit";

};

// who
class Who : public Command {
public:
    string GetName() override;

    bool Execute(const shared_ptr<Message> &msg, Server *server) override;
private:
    string name_ = "who";
};


//send message
class SendMessage : public Command {
public:
    string GetName() override ;

    bool Execute(const shared_ptr<Message> &msg, Server *server) override ;


private:
    string name_ = "message";
};


// 查询历史日志
class History : public  Command {
public:
    string GetName() override ;

    bool Execute(const shared_ptr<Message> &msg, Server *server) override ;

private:
    string name_ = "history";
};

#endif //SERVER_COMMAND_H
