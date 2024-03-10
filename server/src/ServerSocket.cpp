//
// Created by HORIA on 24.01.2024.
//
#include "ServerSocket.h"
#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <cstring>

msd::channel<Message> ServerSocket::messages;
std::unordered_map<std::string, std::shared_ptr<ClientSocket>> ServerSocket::conns;
std::vector<std::thread> ServerSocket::threads;
bool ServerSocket::m_on = true;
int ServerSocket::messageID = 8;

ServerSocket::ServerSocket(int family, int socktype, int protocol, int flags, std::string_view defaultPort,
                           bool blocking) : Socket{family,socktype,protocol,flags,defaultPort}, m_blocking{blocking} {
    if (!m_blocking) {
        u_long mode {1};
        if (ioctlsocket(getSocket(), FIONBIO, &mode) == SOCKET_ERROR) {
            throw Exception {"Error setting server socket to non blocking mode!\n"};
        }
    }
    bindSocket();
    listenSocket();
    m_db = DB::getInstance();
    DB::connect();
}

void ServerSocket::sendAllMessages(const std::shared_ptr<ClientSocket> conn) {
    std::vector<std::string> dbMessages;
    pullMessages(dbMessages);
    for (const auto& message : dbMessages) {
        uint32_t length = htonl(message.size()); // Convert to network byte order (big endian)
        /*
        std::string buffer = std::to_string(length); // no bueno this
        buffer += message;
        std::cout << buffer;
         */
        std::vector<char> buffer(sizeof(length) + message.size());
        std::memcpy(buffer.data(), &length, sizeof(length));
        std::memcpy(buffer.data() + sizeof(length), message.c_str(), message.size());
        uint32_t totalSentBytes = 0;
        while (totalSentBytes < buffer.size()) {
            int sentBytes = send(conn->getSocket(), buffer.data() + totalSentBytes, buffer.size() - totalSentBytes, 0);
            if (sentBytes == -1) {
                // Handle error
                throw std::runtime_error("Error sending message");
            }
            totalSentBytes += sentBytes;
        }
    }
}

void ServerSocket::pullMessages(std::vector<std::string> &dbMessages) {
    DB::pullMessages(dbMessages);
}

void ServerSocket::saveMessage(std::string_view msg) {
    DB::insertMessage(msg, messageID);
    ++messageID;
}

void ServerSocket::serve() {
    while (ServerSocket::m_on) {
        Message msg;
        messages >> msg;
        if (!msg.conn) {
            return;
        }
        std::string ip {Socket::stringifyAdress(msg.conn->getAddr())};
        switch (msg.type) {
            case MessageType::ClientConnected: {
                conns[ip]=msg.conn;
                //send(msg.conn->getSocket(),msg.text.c_str(),msg.text.size(),0);
                sendAllMessages(msg.conn);
                break;
            }
            case MessageType::NewMessage: {
                saveMessage(msg.text);
                std::for_each(conns.begin(),conns.end(),[&msg](auto& pair){
                    if (msg.conn != pair.second) {
                        send(pair.second->getSocket(), msg.text.c_str(), msg.text.size(), 0);
                    }
                });
                break;
            }
            case MessageType::DeleteClient: {
                //conns.erase(ip);
                break;
            }
        }
    }
}

void ServerSocket::acceptSocket(ClientSocket& conn) {
    sockaddr_in clientAddr {};
    int clientAddrSize = sizeof(clientAddr);
    conn.setSocket(accept(Socket::getSocket(), (sockaddr*)&clientAddr, &clientAddrSize));
    conn.setAddr(clientAddr);
}

void ServerSocket::stateCheck() {
    while (ServerSocket::m_on) {
        std::cout << "Type <exit> to close the server\n";
        std::string cmd;
        std::getline(std::cin, cmd);
        std::cout << std::endl;
        if (cmd == "exit") {
            m_on = false;
        }
    }
}

void ServerSocket::run() {
    threads.emplace_back(stateCheck);
    threads.emplace_back(serve);
    ClientSocket conn;
    while (m_on) {
        acceptSocket(conn);
        if (conn.getSocket() != INVALID_SOCKET) {
            auto newConn {std::make_shared<ClientSocket>(conn.getSocket(),conn.getAddr(),false)};
            std::cout << "Accepted connection from: " << Socket::stringifyAdress(newConn->getAddr()) << '\n';
            messages << Message{MessageType::ClientConnected, newConn, "Buna!\n"};
            threads.emplace_back(client,newConn);
        }
    }
    messages.close();
    std::for_each(threads.begin(),threads.end(),[](auto& thr){thr.join();});
    DB::freeInstance();
}

void ServerSocket::client(std::shared_ptr<ClientSocket> conn) {
    char buffer[512];
    int iRes;
    auto defer {dfr::makeDefer([&conn](){ closesocket(conn->getSocket());})};
    do {
        memset(buffer,'\0',512);
        iRes = recv(conn->getSocket(),buffer,512,0);
        if (iRes > 0) {
            messages << Message {MessageType::NewMessage,conn, std::string {buffer}};
        } else if (iRes == 0) {
            messages << Message {MessageType::DeleteClient,conn,std::string {"Client left!\n"}};
            break;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } while (ServerSocket::m_on);
}
