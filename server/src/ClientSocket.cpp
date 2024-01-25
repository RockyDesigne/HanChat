//
// Created by HORIA on 24.01.2024.
//
#include "ClientSocket.h"

ClientSocket::ClientSocket(SOCKET socket, sockaddr_in addr) : m_socket {socket}, m_addr {addr} {}

sockaddr_in ClientSocket::getAddr() const {return m_addr;}

SOCKET ClientSocket::getSocket() const {return m_socket;}

void ClientSocket::setAddr(sockaddr_in addr) {m_addr = addr;}

void ClientSocket::setSocket(SOCKET socket) {m_socket = socket;}

bool ClientSocket::operator!=(const ClientSocket &rhs) {
    return m_socket != rhs.m_socket;
}
