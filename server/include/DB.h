//
// Created by HORIA on 10.03.2024.
//

#ifndef SERVER_DB_H
#define SERVER_DB_H

#include <windows.h>
#include <sql.h>
#include <vector>
#include <string>

class DB {
public:
    enum TableType {
        MESSAGES,
        USERS,
    };
    static void connect();
    static DB* getInstance();
    static void freeInstance();
    static void insertMessage(std::string_view msg, int messageId);
    static void pullMessages(std::vector<std::string> &dbMessages);
private:
    DB();
    ~DB();
    static void extract_error(char *fn,SQLHANDLE handle,SQLSMALLINT type);
    static std::string getInstrStmt(TableType tableType);
    static SQLHANDLE sqlenvhandle;
    static SQLHANDLE sqlconnectionhandle;
    static SQLHANDLE sqlInserthandle;
    static SQLHANDLE sqlPullhandle;
    static SQLRETURN retcode;
    static DB* db;
};

#endif //SERVER_DB_H
