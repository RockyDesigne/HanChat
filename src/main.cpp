#define WIN_32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <exception>
#include <unordered_map>
#include <thread>
#include <list>
#include "channel.h"

class Exceptions : public std::exception {
public:
    explicit Exceptions(std::string_view msg) : m_msg {msg} {}
    [[nodiscard]] const char* what() const noexcept override {return m_msg.data();}
private:
    std::string m_msg {};
};

template<typename F>
class Defer {
public:
    explicit Defer(F f) : m_f{std::move(f)} {} // using move makes sense to capture the lambda, we transfer ownership
    ~Defer() {m_f();}
private:
    F m_f;
};

template<typename Func>
Defer<Func> makeDefer(Func f) {
    return Defer<Func>(std::move(f));
}

class Socket {
public:
    explicit Socket(int family=AF_INET, int socktype=SOCK_STREAM, int protocol=IPPROTO_TCP, int flags=AI_PASSIVE, std::string_view defaultPort="24806") {
        m_hints.ai_family = family;
        m_hints.ai_socktype = socktype;
        m_hints.ai_protocol = protocol;
        m_hints.ai_flags = flags;
        m_port = defaultPort;

        m_errCode = getaddrinfo(nullptr, defaultPort.data(), &m_hints, &m_addrinfo);
        if (m_errCode) {
            throw Exceptions("getaddrinfo failed with error: " + std::to_string(m_errCode));
        }

        m_socket = socket(m_addrinfo->ai_family, m_addrinfo->ai_socktype, m_addrinfo->ai_protocol);

        if (m_socket == INVALID_SOCKET) {
            m_errCode = WSAGetLastError();
            freeaddrinfo(m_addrinfo);
            throw Exceptions("socket failed with error: " + std::to_string(m_errCode));
        }

    }
    ~Socket() {
        closesocket(m_socket);
    }
    //setters
    void setSocket(SOCKET socket) {m_socket = socket;}
    void setErrCode(int errCode) {m_errCode = errCode;}
    //getters
    [[nodiscard]] SOCKET getSocket() const {return m_socket;}
    [[nodiscard]] addrinfo getHints() const {return m_hints;}
    [[nodiscard]] int getErrCode() const {return m_errCode;}
    [[nodiscard]] std::string getPortNum() const {return m_port;}
    //functions
    void bindSocket() {
        m_errCode = bind(m_socket, m_addrinfo->ai_addr, (int)m_addrinfo->ai_addrlen);
        freeaddrinfo(m_addrinfo);
        if (m_errCode == SOCKET_ERROR) {
            closesocket(m_socket);
            throw Exceptions("bind failed with error: " + std::to_string(WSAGetLastError()));
        }
    }
    void listenSocket() {
        m_errCode = listen(m_socket, SOMAXCONN);
        if (m_errCode == SOCKET_ERROR) {
            closesocket(m_socket);
            throw Exceptions("listen failed with error: " + std::to_string(WSAGetLastError()));
        }
        std::cout << "listening on port " << getPortNum() << '\n';
    }

private:
    SOCKET m_socket {INVALID_SOCKET};
    addrinfo* m_addrinfo {};
    addrinfo m_hints {};
    std::string m_port {};
    int m_errCode {};
};

class ClientSocket {
public:
    explicit ClientSocket(SOCKET socket=INVALID_SOCKET, sockaddr_in addr={}) : m_socket {socket}, m_addr{addr} {}
    //getters
    [[nodiscard]] SOCKET getSocket() const {return m_socket;}
    [[nodiscard]] sockaddr_in getAddr() const {return m_addr;}
    //setters
    void setSocket(SOCKET socket) {m_socket = socket;}
    void setAddr(sockaddr_in addr) {m_addr = addr;}
    //functions
private:
    SOCKET m_socket {INVALID_SOCKET};
    sockaddr_in m_addr {};
};

enum class MessageType {
    ClientConnected,
    NewMessage,
    DeleteClient,
};

struct Message {
    MessageType type;
    ClientSocket conn;
    std::string text;
};

class ServerSocket : public Socket {
public:
    explicit ServerSocket(int family=AF_INET, int socktype=SOCK_STREAM, int protocol=IPPROTO_TCP, int flags=AI_PASSIVE, std::string_view defaultPort="24806") : Socket{family,socktype,protocol,flags,defaultPort} {
        bindSocket();
        listenSocket();
    }
    //functions
    static void serve(msd::channel<Message>& messages) {
        std::unordered_map<std::string, ClientSocket> conns;
        while (ServerSocket::m_on) {
            if (!m_on) {
                break;
            }
            Message msg;
            messages >> msg;
            std::string ip {inet_ntoa(msg.conn.getAddr().sin_addr)};
            auto port {msg.conn.getAddr().sin_port};
            switch (msg.type) {
                case MessageType::ClientConnected: {
                    conns[ip+":"+std::to_string(port)]=msg.conn;
                    break;
                }
                case MessageType::NewMessage: {
                    std::for_each(conns.begin(),conns.end(),[&msg](auto& pair){
                        send(pair.second.getSocket(),msg.text.c_str(),msg.text.size(),0);
                    });
                    break;
                }
                case MessageType::DeleteClient: {
                    conns.erase(ip+":"+std::to_string(port));
                    break;
                }
            }
        }
    }
    ClientSocket acceptSocket() {
        SOCKET clientSocket {INVALID_SOCKET};
        sockaddr_in clientAddr {};
        int clientAddrSize = sizeof(clientAddr);
        clientSocket = accept(getSocket(), (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            throw Exceptions("accept failed with error: " + std::to_string(WSAGetLastError()));
        }
        ClientSocket conn{clientSocket, clientAddr};
        m_clients[std::string {inet_ntoa(clientAddr.sin_addr)}]= conn;
        return conn;
    }
    static void stateCheck() {
        std::cout << "Type <exit> to close the server\n";
        std::string cmd;
        std::getline(std::cin, cmd);
        if (cmd == "exit") {
            m_on = false;
        }
    }
    void run() {
        msd::channel<Message> messages;
        std::thread stateT {stateCheck};
        std::thread srvT {serve, std::ref(messages)};
        while (m_on) {
            try {
                ClientSocket conn{acceptSocket()};
                std::cout << "Accepted connection from: " << inet_ntoa(conn.getAddr().sin_addr) << ":"<< conn.getAddr().sin_port << '\n';
                messages << Message {MessageType::ClientConnected, conn, "Buna!\n"};
                std::thread clntT {client,conn,std::ref(messages)};
                clntT.detach();
            } catch (Exceptions &e) {
                std::cerr << e.what() << '\n';
            }
        }
    }

    static void client(ClientSocket conn, msd::channel<Message>& messages) {
        char buffer[512];
        int iRes;
        auto defer {makeDefer([&conn](){ closesocket(conn.getSocket());})};
        do {
            if (!m_on) {
                break;
            }
            memset(buffer,'\0',512);
            iRes = recv(conn.getSocket(),buffer,512,0);
            if (iRes > 0) {
                messages << Message {MessageType::NewMessage,conn, std::string {buffer}};
            }
            else if (iRes == 0) {
                messages << Message {MessageType::DeleteClient, conn, "Connection closed!\n"};
            }
            else {
                messages << Message {MessageType::DeleteClient, conn, "Recv failed!\n"};
            }
        } while (iRes > 0 && ServerSocket::m_on);
    }
private:
    std::unordered_map<std::string, ClientSocket> m_clients;
    static bool m_on;
};
bool ServerSocket::m_on = true;
class Winsock {
public:
    Winsock(const Winsock& winsock)=delete;
    void operator=(const Winsock& winsock)=delete;
    static Winsock* getInstance() {
        if (!m_winsock) {
            try {
                m_winsock = new Winsock {};
            } catch (Exceptions& e) {
                std::cout << "Could not get instance!\n";
                std::cout << e.what() << '\n';
            }
        }
        return m_winsock;
    }
    static void freeInstance() {
        delete m_winsock;
        m_winsock = nullptr;
    }
private:
    WSADATA m_wsadata{};
    static Winsock* m_winsock;
    Winsock() {
        int iRes {WSAStartup(MAKEWORD(2,2),&m_wsadata)};
        if (iRes) {
            throw Exceptions {"WSAStartup failed: " + std::to_string(iRes) + '\n'};
        }
    };
    ~Winsock() {
        WSACleanup();
    }
};

Winsock* Winsock::m_winsock = nullptr;

int main() {
    auto _{makeDefer([](){Winsock::freeInstance(); std::cout << "Winsock freed!\n";})};
    Winsock* winsock {Winsock::getInstance()}; // init winsock

    //create serversocket
    ServerSocket serverSocket {AF_INET,SOCK_STREAM,IPPROTO_TCP,AI_PASSIVE,"24806"};
    serverSocket.run();
    return 0;
}

/*
int main() {

    WSADATA wsaData;

    int iRes {WSAStartup(MAKEWORD(2,2),&wsaData)};
    if (iRes) {
        std::cerr << "WSAStartup failed: " << iRes << '\n';
        return 1;
    }

    constexpr const char* defaultPort {"24806"};

    /*
    // Get the local machine's IP address
    char hostname[NI_MAXHOST];
    iRes = gethostname(hostname, NI_MAXHOST);
    if (iRes != 0) {
        std::cerr << "gethostname failed: " << WSAGetLastError() << '\n';
        WSACleanup();
        return 1;
    }

    std::cout << "host: " << hostname << '\n';


    addrinfo* res {};
    addrinfo hints {AI_PASSIVE,AF_INET,SOCK_STREAM,IPPROTO_TCP};

    iRes = getaddrinfo(nullptr,defaultPort,&hints,&res);
    if (iRes) {
        std::cerr << "getaddrinfo failed: " << iRes << '\n';
        WSACleanup();
        return 1;
    }

    SOCKET listenSocket {socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)};

    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << '\n';
        freeaddrinfo(res);
        WSACleanup();
        return 1;
    }

    iRes = bind(listenSocket,res->ai_addr,(int)res->ai_addrlen);
    if (iRes == SOCKET_ERROR) {
        std::cerr << "bind failed with error: " << WSAGetLastError() << '\n';
        freeaddrinfo(res);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(res);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << '\n';
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    SOCKET clientSocket {INVALID_SOCKET};

    clientSocket = accept(listenSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << '\n';
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    constexpr std::uint16_t defaultBuflen {512};
    char recvbuf[defaultBuflen];
    memset(recvbuf,'\0',defaultBuflen);
    int iSendRes {};
    bool keepGoing {true};
    while (keepGoing) {
        do {
            iRes = recv(clientSocket, recvbuf, defaultBuflen, 0);
            if (iRes > 0) {
                std::cout << "Received bytes: " << iRes << '\n';
                iSendRes = send(clientSocket, recvbuf, iRes, 0);
                if (iSendRes == SOCKET_ERROR) {
                    std::cerr << "send failed: " << WSAGetLastError() << '\n';
                    closesocket(clientSocket);
                }
                std::cout << "bytes sent: " << iSendRes << '\n';
            } else if (!iRes) {
                keepGoing = false;
                std::cout << "Connection closing...\n";
            } else {
                std::cerr << "recv failed: " << WSAGetLastError() << '\n';
                closesocket(clientSocket);
            }
        } while (iRes > 0);
        memset(recvbuf,'\0',defaultBuflen);
        iRes = shutdown(clientSocket, SD_SEND);
        if (iRes == SOCKET_ERROR) {
            std::cerr << "shutdown failed: " << WSAGetLastError() << '\n';
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
*/