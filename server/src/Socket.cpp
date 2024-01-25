//
// Created by HORIA on 24.01.2024.
//
#include "Socket.h"
#include <iostream>

Socket::Socket(int family, int socktype, int protocol, int flags, std::string_view defaultPort) {
    m_hints.ai_family = family;
    m_hints.ai_socktype = socktype;
    m_hints.ai_protocol = protocol;
    m_hints.ai_flags = flags;
    m_port = defaultPort;

    m_errCode = getaddrinfo(nullptr, defaultPort.data(), &m_hints, &m_addrinfo);
    if (m_errCode) {
        throw Exception("getaddrinfo failed with error: " + std::to_string(m_errCode));
    }

    m_socket = socket(m_addrinfo->ai_family, m_addrinfo->ai_socktype, m_addrinfo->ai_protocol);

    if (m_socket == INVALID_SOCKET) {
        m_errCode = WSAGetLastError();
        freeaddrinfo(m_addrinfo);
        throw Exception("socket failed with error: " + std::to_string(m_errCode));
    }

}

Socket::~Socket() {
    closesocket(m_socket);
}

void Socket::setSocket(SOCKET socket) {m_socket = socket;}

void Socket::setErrCode(int errCode) {m_errCode = errCode;}

SOCKET Socket::getSocket() const {return m_socket;}

addrinfo Socket::getHints() const {return m_hints;}

int Socket::getErrCode() const {return m_errCode;}

std::string Socket::getPortNum() const {return m_port;}

void Socket::bindSocket() {
    m_errCode = bind(m_socket, m_addrinfo->ai_addr, (int)m_addrinfo->ai_addrlen);
    freeaddrinfo(m_addrinfo);
    if (m_errCode == SOCKET_ERROR) {
        closesocket(m_socket);
        throw Exception("bind failed with error: " + std::to_string(WSAGetLastError()));
    }
}

void Socket::listenSocket() {
    m_errCode = listen(m_socket, SOMAXCONN);
    if (m_errCode == SOCKET_ERROR) {
        closesocket(m_socket);
        throw Exception("listen failed with error: " + std::to_string(WSAGetLastError()));
    }
    std::cout << "listening on port " << getPortNum() << '\n';
}

std::string Socket::stringifyAdress(const sockaddr_in& addr) {
    std::string adrIp {inet_ntoa(addr.sin_addr)};
    adrIp += ":" + std::to_string(addr.sin_port);
    return adrIp;
}
