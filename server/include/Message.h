//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include "ClientSocket.h"
#include <string>
#include <memory>

enum class MessageType {
    ClientConnected,
    NewMessage,
    DeleteClient,
};

struct Message {
    MessageType type;
    std::shared_ptr<ClientSocket> conn{};
    std::string text;
};


#endif //SERVER_MESSAGE_H
