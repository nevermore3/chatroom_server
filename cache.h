//
// Created by jmq on 2020/4/10.
//

#ifndef SERVER_CACHE_H
#define SERVER_CACHE_H

#include <hiredis/hiredis.h>
#include <memory>
#include "common.h"
#include <map>
using namespace std;

#define KEY "info"

struct Message;

class Cache {
public:
    static Cache *GetInstance() {
        static Cache instance_;
        return &instance_;
    }

    Cache();

    void SetConfig(int dbName, const char *key);


    int SetID(const char *key);

    string GetID();

    int HashHmset(string &key, const map<string, string> &values);

    void SetValues(map<string, string> values);

    void Insert(shared_ptr<Message> &msg);
private:

    int dbName_;

    // 哈希表存储
    const char *key_;

    // 保存与redis数据库的链接
    redisContext *connection_;

    // 保存redis操作的结果
    redisReply *reply_;


};


#endif //SERVER_CACHE_H
