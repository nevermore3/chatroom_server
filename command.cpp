//
// Created by jmq on 2020/4/13.
//

#include "command.h"

void CommandManager::Register(string name, shared_ptr<Command> command) {

}

void CommandManager::Register(shared_ptr<Command> command) {

}

CommandManager::CommandManager() {

}

bool CommandManager::Execute(char *msg) {
    return false;
}

bool Who::Execute(char *msg) {
    return false;
}

string Who::GetName() {
    return std::__cxx11::string();
}

string Exit::GetName() {
    return std::__cxx11::string();
}

bool Exit::Execute(char *msg) {
    return false;
}
