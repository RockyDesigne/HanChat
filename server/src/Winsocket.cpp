//
// Created by HORIA on 24.01.2024.
//
#include "Winsocket.h"
Winsocket* Winsocket::m_winsock = nullptr;
Winsocket *Winsocket::getInstance() {
    if (!m_winsock) {
        try {
            m_winsock = new Winsocket {};
        } catch (Exception& e) {
            std::cout << "Could not get instance!\n";
            std::cout << e.what() << '\n';
        }
    }
    return m_winsock;
}

void Winsocket::freeInstance() {
    delete m_winsock;
    m_winsock = nullptr;
}

Winsocket::Winsocket() {
    int iRes {WSAStartup(MAKEWORD(2,2),&m_wsadata)};
    if (iRes) {
        throw Exception {"WSAStartup failed: " + std::to_string(iRes) + '\n'};
    }
}

Winsocket::~Winsocket() {
    WSACleanup();
}
