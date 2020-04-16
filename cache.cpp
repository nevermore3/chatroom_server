//
// Created by jmq on 2020/4/10.
//

#include "cache.h"

// 用于自增计数的key
#define ID "ID"

Cache::Cache() {
    connection_ = redisConnect(REDIS_HOST, REDIS_PORT);
    if (connection_->err) {
        perror("Redis connection error");
        redisFree(connection_);
        exit(-1);
    }
    SetConfig(0, KEY);

    cout << "Redis init Over" << endl;
}

void Cache::SetConfig(int dbName, const char *key) {
    dbName_ = dbName;
    key_ = key;
}

int Cache::HashHmset(string &key, const map<string, string> &values) {

    /*
     *  利用redis中的 string数据结构的自增特性
     *  建立一个 string  类型的数据结构， value存储id值，每次自增
     *  然后 和KEY+ id 可以保证 hash表的 key值是唯一的
     */
    for (const auto& iter : values) {
        reply_ = (redisReply *)redisCommand(connection_, "HSET %s %s %s",
                key.c_str(), iter.first.c_str(), iter.second.c_str());
        if (reply_->type == REDIS_REPLY_ERROR) {
            perror("hmset error");
            return -1;
        }
        freeReplyObject(reply_);
    }
    cout << "Set  hash key : " << key << "  ok" <<endl;
    return 1;
}

int Cache::SetID(const char *key) {

    reply_ = (redisReply *) redisCommand(connection_, "INCR %s", key);
    if (reply_ == nullptr) {
        perror ("Set ID INCR error");
        return -1;
    }
    freeReplyObject(reply_);
    return 1;

}

string Cache::GetID() {
    reply_ = (redisReply *) redisCommand(connection_, "GET %s", ID);
    if (reply_ == nullptr) {
        perror("GET ID error");
        return "-1";
    }
    return string(reply_->str);
}

void Cache::SetValues(map<string, string> values) {
    SetID(ID);
    string keyName = string(ID) + GetID();
    HashHmset(keyName, values);
}



void Cache::Insert(shared_ptr<Message> &msg) {
    map<string,string> info;
    info.insert(make_pair("fromID", to_string(msg->fromID_)));
    info.insert(make_pair("toID", to_string(msg->toID_)));
    info.insert(make_pair("message", string(msg->content_)));
    SetValues(info);
}
