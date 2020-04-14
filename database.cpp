//
// Created by jmq on 2020/4/9.
//

#include "database.h"
#include "common.h"
#include <ctime>

void DataBase::InitDB() {
    // 建立数据库链接
    connection_ = mysql_real_connect(connection_, host_, user_, passWord_, dbName_, 0, nullptr, 0);
    if (connection_ == nullptr) {
        perror("mysql connection error");
        exit(-1);
    }
    // 创建数据table
    CreateTable(TABLE_NAME);
    cout << "DataBase init.." << endl;

//    string msg = "hello";
//    Insert(1, 2, msg, const_cast<char*>(TABLE_NAME));
//    cout<<FromLog(1)<<endl;
}




DataBase::~DataBase() {
    if (connection_ != nullptr) {
        mysql_close(connection_);
    }
}

bool DataBase::CreateTable(const char *tableName) {
    // 判断表是否存在若不存在，否则创建表
    /*
     * 表结构如下
     * --ID---FromFD---ToFD--Message---TimeStamp---Result---
     */
    char buff[BUF_SIZE] = {0};
    bzero(buff, BUF_SIZE);
    sprintf(buff, "create table if not exists %s(\
            id int not null AUTO_INCREMENT,\
            fromID int not null,\
            toID int not null,\
            message text not null,\
            time char(30) not null,\
            primary key(id));", tableName);

    ExeSQL(buff);
    return true;
}

void DataBase::Insert(int fromId, int toId, string &message, char *tableName) {
    char insertBuf[BUF_SIZE] = {0};
    bzero(insertBuf, BUF_SIZE);

    time_t curTime;
    time(&curTime);

    sprintf(insertBuf, "insert into %s(fromId, toId, message, time)\
                       values(%d, %d, \"%s\", \"%s\");", tableName, fromId, toId, message.c_str(), ctime(&curTime));
    //cout<<insertBuf<<endl;
    ExeSQL(insertBuf);

}

string DataBase::ExeSQL(char *sql) {
    if (sql == nullptr) {
        perror("ExeSQL sql is nullptr");
        return string();
    }

    {
        // 执行sql语句时，加锁，避免多线程问题
        unique_lock<mutex> queryLock(queryMutex_);
        if (mysql_query(connection_, sql)) {
            perror("query error");
            return string();
        }
    }


    // 获取查询结果
    result_ = mysql_store_result(connection_);
    string result;
    // 最大查询NUM条记录
    const int NUM = 10;
    if (result_ != nullptr) {
        unsigned int cols = mysql_num_fields(result_);
        unsigned int rows = mysql_num_rows(result_);

        for (unsigned int i = 0; i < rows && i < NUM; i++) {
            // 获取一行数据
            row_ = mysql_fetch_row(result_);
            if (row_ == nullptr) {
                break;
            }

            for (unsigned int j = 0; j < cols; j++) {
                result += row_[j];
                if (j != cols - 1) {
                    result += "\t";
                }
            }
            if (i != rows - 1 || i != NUM - 1) {
                result += "\n";
            }
        }
    } else {
        // 执行的是insert update delete类的非查询语句
        if (mysql_field_count(connection_) == 0) {
            cout << "Excute insert | update | delete " << endl;
        } else {
            perror("Get result error");
            exit(-1);
        }
    }
    return  result;
}


string DataBase::FromLog(int fromId) {
    char buff[BUF_SIZE] = {0};
    bzero(buff, BUF_SIZE);
    sprintf(buff, "select message from info where fromID = %d;", fromId);
    return ExeSQL(buff);
}

string DataBase::ToLog(int toId) {
    char buff[BUF_SIZE] = {0};
    bzero(buff, BUF_SIZE);
    sprintf(buff, "select message from info where toID = %d or toID = -1;", toId);
    return ExeSQL(buff);
}

string DataBase::AllLog() {
    char buff[BUF_SIZE] = {0};
    bzero(buff, BUF_SIZE);
    sprintf(buff, "select message from info;");
    return ExeSQL(buff);
}
