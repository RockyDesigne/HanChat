//
// Created by HORIA on 24.01.2024.
//
#include "ServerSocket.h"
#include <iostream>
#include <chrono>

msd::channel<Message> ServerSocket::messages;
std::string ServerSocket::m_clientConnectedMessage {"Welcome to HanChat!\n"};
bool ServerSocket::m_on = true;
std::queue<ServerSocket::clientId> ServerSocket::clients;
std::vector<std::thread> ServerSocket::threads;
std::mutex ServerSocket::Worker::m_mutex;
std::vector<ClientSocket> ServerSocket::m_clientsVec;
std::vector<ServerSocket::Worker>ServerSocket::m_workers(nrOfWorkers);
std::condition_variable ServerSocket::Worker::m_cv;

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
}

void ServerSocket::serve() {
    while (ServerSocket::m_on) {
        Message msg;
        messages >> msg;
        if (m_clientsVec.empty() || msg.clientId > m_clientsVec.size()) {
            return;
        }
        auto conn {m_clientsVec[msg.clientId]};
        switch (msg.type) {
            case MessageType::ClientConnected: {
                send(conn.getSocket(),msg.text.c_str(),msg.text.size(),0);
                break;
            }
            case MessageType::NewMessage: {
                for (int i {}; i < m_clientsVec.size(); ++i) {
                    if (i != msg.clientId) {
                        send(m_clientsVec[i].getSocket(),msg.text.c_str(),msg.text.size(),0);
                    }
                }
                break;
            }
            case MessageType::DeleteClient: {
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
    threads.emplace_back(manager);

    ClientSocket conn;
    int id = 0;
    while (m_on) {
        acceptSocket(conn);
        if (conn.getSocket() != INVALID_SOCKET) {
            m_clientsVec.push_back(conn);
            std::cout << "Accepted connection from: " << Socket::stringifyAdress(conn.getAddr()) << '\n';
            messages << Message{MessageType::ClientConnected, id, ServerSocket::m_clientConnectedMessage};
            clients.push(id);
            ++id;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    messages.close();
    std::for_each(m_workers.begin(),m_workers.end(),[](auto& worker){worker.m_thread.join();});
    std::for_each(threads.begin(),threads.end(),[](auto& thr){thr.join();});
}

void ServerSocket::handleClient(bool &done, ServerSocket::clientId &id) {
    char buffer[512];
    int iRes;
    while (ServerSocket::m_on) {
        if (done)
        {
            std::unique_lock<std::mutex> lk{ServerSocket::Worker::m_mutex};
            ServerSocket::Worker::m_cv.wait(lk, [&done] { return (!done || !ServerSocket::m_on); });
            if (!ServerSocket::m_on) {
                return;
            }
            if ((m_clientsVec.empty() || id > m_clientsVec.size())) {
                throw Exception {"Client ID is out of bounds in handleClient!\n"};
            }
        }
        //printf("Here!, l 113\n");
        const auto& conn = m_clientsVec[id];
        memset(buffer,'\0',512);
        iRes = recv(conn.getSocket(),buffer,512,0);
        //printf("tried recv!\n");
        if (iRes > 0) {
            messages << Message {MessageType::NewMessage, id, std::string {buffer}};
        } else {
            //printf("recvd 0!\n");
            {
                std::unique_lock<std::mutex> lk{ServerSocket::Worker::m_mutex};
                clients.push(id);
            }
            //printf("Done client with id:%i!\n", id);
            done = true;
        }
    }
}

void ServerSocket::assignClient(ServerSocket::clientId id) {
    bool found {false};
    int i=0;
    while (!found) { //blocks the manager thread if no available workers
        if (i == m_workers.size()) {
            i = 0;
        }
        if (m_workers[i].m_done) {
            found = true;
            m_workers[i].m_id = id; // assign the new client
            m_workers[i].m_done = false; // tell worker task needs doing
            Worker::m_cv.notify_one();
        }
        ++i;
    }
}

void ServerSocket::manager() {
    while (ServerSocket::m_on) {
        if (clients.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        //printf("assign called!\n");
        assignClient(clients.front());
        clients.pop();
    }
    Worker::m_cv.notify_all();
}
