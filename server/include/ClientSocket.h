//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_CLIENTSOCKET_H
#define SERVER_CLIENTSOCKET_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>

class ClientSocket {
public:
    ClientSocket(SOCKET socket=INVALID_SOCKET, sockaddr_in addr={});
    //getters
    [[nodiscard]] sockaddr_in getAddr() const;
    SOCKET getSocket() const;
    //setters
    void setAddr(sockaddr_in addr);
    void setSocket(SOCKET socket);
    //functions
    bool operator!=(const ClientSocket& rhs);
private:
    SOCKET m_socket {};
    sockaddr_in m_addr {};
};

#endif //SERVER_CLIENTSOCKET_H