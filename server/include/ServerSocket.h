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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <unordered_map>
#include <thread>

class ServerSocket : public Socket {
public:
    using clientId = int;
    struct Worker {
        bool m_done {true};
        static std::condition_variable m_cv;
        static std::mutex m_mutex;
        clientId m_id {0};
        std::thread m_thread {ServerSocket::handleClient, std::ref(m_done), std::ref(m_id)};
    };
    explicit ServerSocket(int family=AF_INET, int socktype=SOCK_STREAM, int protocol=IPPROTO_TCP, int flags=AI_PASSIVE, std::string_view defaultPort="24806", bool blocking=false);
    //functions
    static void serve();
    void acceptSocket(ClientSocket& conn);
    static void stateCheck();
    void run();
    static void handleClient(bool& done, ServerSocket::clientId& id);
    static void assignClient(ServerSocket::clientId id);
    static void manager();

private:
    bool m_blocking;
    static msd::channel<Message> messages;
    //static std::unordered_map<std::string, std::shared_ptr<ClientSocket>> conns;
    static std::vector<ClientSocket> m_clientsVec;
    static std::vector<std::thread> threads;
    static std::vector<ServerSocket::Worker> m_workers;
    static std::queue<ServerSocket::clientId> clients;
    static constexpr int nrOfWorkers {3};

    static bool m_on;
    static std::string m_clientConnectedMessage;
};

#endif //SERVER_SERVERSOCKET_H
