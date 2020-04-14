//
// Created by jmq on 2020/4/13.
//

#include "utility.h"
#include "command.h"

shared_ptr<Message> ParseData(int clientFd, string data) {
    shared_ptr<Message> message = make_shared<Message>();
    message->fromID_ = clientFd;

    if (data[0] != '\\') {
        message->toID_ = - 1;
        strncpy(message->content_, const_cast<char*>(data.c_str()), BUF_SIZE - 1);
        return message;
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
    string info;
    for (unsigned i = 1; i < tokens.size(); i++) {
        info += tokens[i];
        if (i != tokens.size() - 1) {
            info += " ";
        }
    }

    if (tokens[0] == "ALL" || tokens[0] == "all") {
        message->toID_ = -1;
    } else {
        message->toID_ = atoi(tokens[0].c_str());
    }

    strncpy(message->content_, const_cast<char*> (info.c_str()), BUF_SIZE - 1);

    return message;
}