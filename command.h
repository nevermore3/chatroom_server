//
// Created by jmq on 2020/4/13.
//

#ifndef SERVER_COMMAND_H
#define SERVER_COMMAND_H

#include <memory>
#include "common.h"

using namespace std;
// 观察着模式，用于处理客户端发送过来的命令

class Command {
public:
    virtual ~Command() = default;

    virtual bool Execute(char *msg) = 0;

    virtual string GetName() = 0;
};


class CommandManager {
public:
    CommandManager();

    void Register(string name, shared_ptr<Command> command);

    void Register(shared_ptr<Command> command);


    bool Execute(char *msg);

private:
    // 所有注册的命令
    vector<shared_ptr<Command>> commands_;
};


/*
 *  命令集合
 */

// exit
class Exit : public Command {
public:
    string GetName() override;

    bool Execute(char *msg) override;

private:
    string name = "exit";

};

// who
class Who : public Command {
public:
    string GetName() override;

    bool Execute(char *msg) override;
private:
    string name = "who";
};

//


#endif //SERVER_COMMAND_H
