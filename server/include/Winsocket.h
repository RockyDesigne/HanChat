//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_WINSOCKET_H
#define SERVER_WINSOCKET_H

#include "My_Exception.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>

#include <iostream>

class Winsocket {
public:
    Winsocket(const Winsocket& winsock)=delete;
    void operator=(const Winsocket& winsock)=delete;
    static Winsocket* getInstance();
    static void freeInstance();
private:
    WSADATA m_wsadata{};
    static Winsocket* m_winsock;
    Winsocket();
    ~Winsocket();
};

#endif //SERVER_WINSOCKET_H
