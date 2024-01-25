//
// Created by HORIA on 24.01.2024.
//
#include "ServerSocket.h"
#include <iostream>

msd::channel<Message> ServerSocket::messages;
std::unordered_map<std::string, ClientSocket> ServerSocket::conns;
std::vector<std::thread> ServerSocket::threads;
bool ServerSocket::m_on = true;

ServerSocket::ServerSocket(int family, int socktype, int protocol, int flags, std::string_view defaultPort,
                           bool blocking) : Socket{family,socktype,protocol,flags,defaultPort}, m_blocking{blocking} {
    if (!blocking) {
        u_long mode {1};
        if (ioctlsocket(getSocket(), FIONBIO, &mode) == SOCKET_ERROR) {
            throw Exception {"Error setting server socket to non blocking mode!\n"};
        }
    }
    bindSocket();
    listenSocket();
}

void ServerSocket::serve() {
    while (ServerSocket::m_on) {
        Message msg;
        messages >> msg;
        std::string ip {Socket::stringifyAdress(msg.conn.getAddr())};
        switch (msg.type) {
            case MessageType::ClientConnected: {
                conns[ip]=msg.conn;
                send(msg.conn.getSocket(),msg.text.c_str(),msg.text.size(),0);
                break;
            }
            case MessageType::NewMessage: {
                std::for_each(conns.begin(),conns.end(),[&msg](auto& pair){
                    if (msg.conn != pair.second) {
                        send(pair.second.getSocket(), msg.text.c_str(), msg.text.size(), 0);
                    }
                });
                break;
            }
            case MessageType::DeleteClient: {
                conns.erase(ip);
                break;
            }
        }
    }
}

ClientSocket ServerSocket::acceptSocket() {
    sockaddr_in clientAddr {};
    int clientAddrSize = sizeof(clientAddr);
    ClientSocket conn {accept(Socket::getSocket(), (sockaddr*)&clientAddr, &clientAddrSize),clientAddr};
    return conn;
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
    while (m_on) {
        ClientSocket conn{acceptSocket()};
        if (conn.getSocket() != INVALID_SOCKET) {
            std::cout << "Accepted connection from: " << Socket::stringifyAdress(conn.getAddr()) << '\n';
            messages << Message{MessageType::ClientConnected, conn, "Buna!\n"};
            threads.emplace_back(client,conn);
        }
    }
    messages.close();
    std::for_each(threads.begin(),threads.end(),[](auto& thr){thr.join();});
}

void ServerSocket::client(ClientSocket conn) {
    char buffer[512];
    int iRes;
    auto defer {dfr::makeDefer([&conn](){ closesocket(conn.getSocket());})};
    do {
        memset(buffer,'\0',512);
        iRes = recv(conn.getSocket(),buffer,512,0);
        if (iRes > 0) {
            messages << Message {MessageType::NewMessage,conn, std::string {buffer}};
        }
    } while (ServerSocket::m_on);
}
