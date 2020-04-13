//
// Created by jmq on 2020/4/13.
//

#include "command.h"
#include "utility.h"
#include "server.h"

void CommandManager::Register(const string& name, const shared_ptr<Command>& command) {
    if (command == nullptr || name.length() == 0) {
        return;
    }

    // 判断是否有同名的命令存在,如果有则替换，否则插入
    if (nameCommands_.find(name) != nameCommands_.end()) {
        shared_ptr<Command> oldCommand = nameCommands_[name];
        for (unsigned int i = 0; i < commands_.size(); i++) {
            if (oldCommand == commands_[i]) {
                commands_[i] = command;
                break;
            }
        }
        nameCommands_[name] = command;
    } else {
        nameCommands_.insert(make_pair(name, command));
        commands_.emplace_back(command);
    }
}

void CommandManager::Register(const shared_ptr<Command>& command) {
    if (command == nullptr) {
        return;
    }
    Register(command->GetName(), command);
}

void CommandManager::RemoveCommand(string name) {
    if (nameCommands_.find(name) == nameCommands_.end()) {
        return;
    }
    shared_ptr<Command>command = nameCommands_[name];
    nameCommands_.erase(name);
    for (auto iter = commands_.begin(); iter != commands_.end(); iter++) {
          if (*iter == command) {
              commands_.erase(iter);
              break;
          }
    }
}

bool CommandManager::Execute(const shared_ptr<Message>& msg, Server *server) {

    for (const auto& command : commands_) {
        if (command != nullptr && !strcmp(msg->content_, command->GetName().c_str())) {
            cout << "Command [" << command->GetName() << "] Start. " << endl;
            return command->Execute(msg, server);
        }
    }
    // 都匹配不上，则发送消息
    if (nameCommands_.find("message") == nameCommands_.end()) {
        perror("Not Register command : message");
    } else {
        cout << "Command [ SendMessage ] Start. " << endl;
        nameCommands_["message"]->Execute(msg, server);
    }
    return true;
}



bool Who::Execute(const shared_ptr<Message>& msg, Server *server) {
    // 向客户端发送当前在chat room的所有fd
    bool flag = true;
    string info = "当前聊天室有 : ";
    for (auto fd : server->GetChatRoom()) {
        if (fd == msg->fromID_) {
            continue;
        }
        flag = false;
        info += to_string(fd);
        info += "\t";
    }
    if (flag) {
        info += " 就你自己 ：";
        info += to_string(msg->fromID_);
    }
    if (send(msg->fromID_, info.c_str(), info.size(), 0) < 0) {
        perror("send welcome error");
        exit(-1);
    }
    return true;
}


string Who::GetName() {
    return name_;
}

string Exit::GetName() {
    return name_;
}

bool Exit::Execute(const shared_ptr<Message> &msg, Server *server) {
    cout << msg->fromID_ << " 打出exit, 并中断了与服务器的链接" << endl;
    return false;
}

string SendMessage::GetName() {
    return name_;
}

bool SendMessage::Execute(const shared_ptr<Message> &msg, Server *server) {
    if (msg->toID_ == -1) {
        server->SendMessageToAll(msg->fromID_, msg->content_);
    } else {
        server->SendMessageToOne(msg->fromID_, msg->toID_, msg->content_);
    }
    cout << msg->fromID_ << " say : " << msg->content_ << endl;
    return true;
}


string History::GetName() {
    return name_;
}

bool History::Execute(const shared_ptr<Message> &msg, Server *server) {
    char buff[BUF_SIZE * 2] = {0};
    bzero(buff, BUF_SIZE);
    sprintf(buff, "select message from info where fromID = %d;", msg->fromID_);
    string log = server->GetDB()->ExeSQL(buff);
    server->SendMessageToOne(msg->fromID_, msg->fromID_, const_cast<char*>(log.c_str()));
    return true;
}
