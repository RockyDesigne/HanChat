//
// Created by HORIA on 10.03.2024.
//
#include "DB.h"
#include "My_Exception.h"

#include <sqlext.h>
#include <iostream>
#include "Defer.h"

DB* DB::db = nullptr;
SQLHANDLE DB::sqlenvhandle = nullptr;
SQLHANDLE DB::sqlconnectionhandle = nullptr;
SQLHANDLE DB::sqlInserthandle = nullptr;
SQLHANDLE DB::sqlPullhandle = nullptr;
SQLRETURN DB::retcode = 0;

void DB::extract_error(char *fn, SQLHANDLE handle, SQLSMALLINT type) {
    SQLINTEGER   i = 0;
    SQLINTEGER   native;
    SQLCHAR      state[ 7 ];
    SQLCHAR      text[256];
    SQLSMALLINT  len;
    SQLRETURN    ret;

    fprintf(stderr,
            "\n"
            "The driver reported the following diagnostics whilst running "
            "%s\n\n",
            fn);

    do
    {
        i += 1;
        ret = SQLGetDiagRec(type, handle, (SQLSMALLINT) i, state, &native, text,
                            sizeof(text), &len );
        if (SQL_SUCCEEDED(ret))
            printf("%s:%ld:%ld:%s\n", state, i, native, text);
    }
    while( ret == SQL_SUCCESS );
}

DB::DB() {}

DB::~DB() {
    SQLDisconnect(sqlconnectionhandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlconnectionhandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlenvhandle);
    if (sqlPullhandle)
        SQLFreeHandle(SQL_HANDLE_STMT, sqlPullhandle); // no memory leak pls
    if (sqlInserthandle)
        SQLFreeHandle(SQL_HANDLE_STMT, sqlInserthandle); // no memory leak pls
}

DB* DB::getInstance() {
    if (!db) {
        db = new DB{};
    }
    return db;
}

void DB::freeInstance() {
    delete db;
    db = nullptr;
}

std::string DB::getInstrStmt(TableType tableType) {
    switch (tableType) {
        case DB::MESSAGES: return "Messages";
        case DB::USERS: return "Users";
        default: throw Exception {"NO SUCH TABLE IN DB!"};
    }
}

void DB::pullMessages(std::vector<std::string> &dbMessages) {

    auto defer {dfr::makeDefer([](){ SQLFreeHandle(SQL_HANDLE_STMT,sqlPullhandle);})};

    if(SQL_SUCCESS!=SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlPullhandle))
        throw Exception {"Error alloc Pull handle"};

    if (SQLPrepare(sqlPullhandle, (SQLCHAR*)"SELECT MessageText FROM Messages", SQL_NTS) != SQL_SUCCESS) // compiles the command, useful 'cause we use it up to n times
        throw Exception {"Error in preparing stmt for pullMessages!"};

    if (SQLExecute(sqlPullhandle) != SQL_SUCCESS)
        throw Exception {"Error executing pullMessages!"};

    char res[257]; //+1 for null termination, otherwise, it the SQLFetch fails
    SQLLEN result_len;
    if (
    SQLBindCol(sqlPullhandle, 1, SQL_C_CHAR, res, sizeof(res), &result_len) != SQL_SUCCESS)
        throw Exception {"Error in binding col"};

    //sqlfetch gets the next row from the result table and stores it in the buffer that we bound with SQLBindCol
    while (SQLFetch(sqlPullhandle) == SQL_SUCCESS) {
        dbMessages.emplace_back(res);
    }

}

void DB::insertMessage(std::string_view msg, int messageId) {
    std::string stmt {"INSERT INTO Messages (MessageID, UserID, MessageText) "
                      "VALUES ("};
    stmt += std::to_string(messageId);
    stmt += ',';
    stmt += " 1, ";
    stmt += "'";
    stmt += msg;
    stmt += "')";

    retcode = SQLExecDirect(sqlInserthandle, (SQLCHAR*)stmt.c_str(), SQL_NTS);
    if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        // Handle error
        SQLCHAR sqlstate[6];
        SQLINTEGER nativeerror;
        SQLCHAR message[SQL_MAX_MESSAGE_LENGTH];
        SQLSMALLINT messagelen;
        SQLGetDiagRec(SQL_HANDLE_STMT, sqlInserthandle, 1, sqlstate, &nativeerror, message, sizeof(message), &messagelen);
        printf("Error: %s\n", message);
        throw Exception {"Error executing insertMessage!"};
    }
}

void DB::connect() {

    if(SQL_SUCCESS!=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlenvhandle))
        throw Exception{"Failed alloc environment handle"};

    if(SQL_SUCCESS!=SQLSetEnvAttr(sqlenvhandle,SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0))
        throw Exception{"Failed setting the environment handle"};

    if(SQL_SUCCESS!=SQLAllocHandle(SQL_HANDLE_DBC, sqlenvhandle, &sqlconnectionhandle))
        throw Exception{"Failed alloc connection handle"};

    SQLCHAR retconstring[1024];
    switch(SQLDriverConnect (sqlconnectionhandle,
                             NULL,
                             (SQLCHAR*)"DRIVER={ODBC Driver 17 for SQL Server};SERVER=HARRYPC,1433;DATABASE=HanStorage;UID=HARRYPC\HORIA;Trusted_Connection=yes;",
                             SQL_NTS,
                             retconstring,
                             1024,
                             NULL,
                             SQL_DRIVER_NOPROMPT)) {
        case SQL_SUCCESS_WITH_INFO:
            printf("Successfully connected to database. Returned connection string was:\n\t%s\n", retconstring);
            extract_error("SQLDriverConnect", sqlconnectionhandle, SQL_HANDLE_DBC);
            break;
        case SQL_SUCCESS:
            printf("Successfully connected to database. Returned connection string was:\n\t%s\n", retconstring);
            break;
        case SQL_INVALID_HANDLE:
        case SQL_ERROR:
            extract_error("SQLDriverConnect", sqlconnectionhandle, SQL_HANDLE_DBC);
            throw Exception{"\nSQLDriverConnect failed!"};
        default:
            break;
    }

    if(SQL_SUCCESS!=SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlInserthandle))
        throw Exception {"Error alloc Insert handle"};
}