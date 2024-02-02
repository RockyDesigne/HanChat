//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include "My_Exception.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

class Socket {
public:
    explicit Socket(int family=AF_INET, int socktype=SOCK_STREAM, int protocol=IPPROTO_TCP, int flags=AI_PASSIVE, std::string_view defaultPort="24806");
    ~Socket();
    //setters
    void setSocket(SOCKET socket);
    void setErrCode(int errCode);
    //getters
    [[nodiscard]] SOCKET getSocket() const;
    [[nodiscard]] addrinfo getHints() const;
    [[nodiscard]] int getErrCode() const;
    [[nodiscard]] std::string getPortNum() const;
    //functions
    void bindSocket();
    void listenSocket();
    static std::string stringifyAdress(const sockaddr_in& addr);
private:
    SOCKET m_socket {INVALID_SOCKET};
    addrinfo* m_addrinfo {};
    addrinfo m_hints {};
    std::string m_port {};
    int m_errCode {};
};

#endif //SERVER_SOCKET_H
