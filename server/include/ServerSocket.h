//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_SERVERSOCKET_H
#define SERVER_SERVERSOCKET_H

#include "Socket.h"
#include "Message.h"
#include "Defer.h"
#include "channel.h"
#include "ClientSocket.h"
#include "DB.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <unordered_map>
#include <thread>

class ServerSocket : public Socket {
public:
    explicit ServerSocket(int family=AF_INET, int socktype=SOCK_STREAM, int protocol=IPPROTO_TCP, int flags=AI_PASSIVE, std::string_view defaultPort="24806", bool blocking=false);
    //functions
    static void serve();
    void acceptSocket(ClientSocket& conn);
    static void stateCheck();
    void run();
    static void sendAllMessages(const std::shared_ptr<ClientSocket> conn);
    static void saveMessage(std::string_view msg);
    static void pullMessages(std::vector<std::string>& dbMessages);

    static void client(std::shared_ptr<ClientSocket> conn);
private:
    bool m_blocking;
    static msd::channel<Message> messages;
    static std::unordered_map<std::string, std::shared_ptr<ClientSocket>> conns;
    static std::vector<std::thread> threads;
    static bool m_on;
    static int messageID;
    DB* m_db {};
};

#endif //SERVER_SERVERSOCKET_H
