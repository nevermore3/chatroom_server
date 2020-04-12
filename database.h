//
// Created by jmq on 2020/4/9.
//

#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#include <mysql/mysql.h>
#include <string>
#include "common.h"
using namespace std;
class DataBase {
public:
    DataBase() : user_(DATABASE_ACCOUT), passWord_(DATABASE_PW), dbName_(DB_NAME), host_(HOST) {
        connection_ = mysql_init(nullptr);
        if (connection_ == nullptr) {
            perror("mysql init  error");
            exit(-1);
        }
    };

    ~DataBase();

    bool CreateTable(const char *tableName);

    void InitDB();

    void Insert(int fromId, int toId, string &message, char *tableName);

    string FromLog(int fromId);

    string ToLog(int toId);

    string AllLog();

    string ExeSQL(char *sql);

private:
    const char *user_;

    const char *passWord_;

    const char *host_;

    const char *dbName_;


    MYSQL *connection_;

    MYSQL_RES *result_;

    MYSQL_ROW  row_;

    // 多线程查询数据库时，加锁
    mutex queryMutex_;

};


#endif //SERVER_DATABASE_H
