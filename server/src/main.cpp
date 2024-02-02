#include "Winsocket.h"
#include "ServerSocket.h"

int main() {

    //init Winsocket
    Winsocket::getInstance();

    //create serversocket
    ServerSocket serverSocket {AF_INET,SOCK_STREAM,IPPROTO_TCP,AI_PASSIVE,"24806",false};
    serverSocket.run();

    Winsocket::freeInstance();
    return 0;
}
