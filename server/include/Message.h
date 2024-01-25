//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include "ClientSocket.h"
#include <string>

enum class MessageType {
    ClientConnected,
    NewMessage,
    DeleteClient,
};

struct Message {
    MessageType type;
    ClientSocket conn;
    std::string text;
};


#endif //SERVER_MESSAGE_H
