
#if defined(_WIN32)
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#if defined(_WIN32)           // raylib uses these names as function parameters
#undef near
#undef far
#endif
// TO DO:
// Fix the way messages resize, they resize badly when in fullscreen, they dont change
// make it so you dont have to press backspace repetitively to delete a message
// add a cursor so ypou can switch between characters
// add support for UTF-8 and arabic font
#include <raylib.h>

#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <string>

#include "GUI.h"

#define DEFAULT_BUFLEN 512
std::string DEFAULT_PORT  = "24806";

static std::string serverIpAdressAndPort;

static bool on {true};
char recvbuf[DEFAULT_BUFLEN];
void rec(SOCKET& sock, std::list<client::MessageBox>& message_boxes, Rectangle& box) {
    while (on) {
        int i = recv(sock, recvbuf, DEFAULT_BUFLEN, 0);
        if (i > 0) {
            client::MessageBox message_box{box.x + 50, box.y - 50 * (message_boxes.size() + 1), box.width - 50, 50,recvbuf};
            message_boxes.push_back(message_box);
            std::memset(recvbuf, '\0', DEFAULT_BUFLEN);
        }
    }
}

int main()
{
    std::memset(recvbuf, '\0', DEFAULT_BUFLEN);
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
            *ptr = NULL,
            hints;
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::cout << "Input address and port number of the server: \n";
    std::cin >> serverIpAdressAndPort;

    int pos = serverIpAdressAndPort.find(":");
    DEFAULT_PORT = serverIpAdressAndPort.substr(pos+1);
    std::string ipAddress = serverIpAdressAndPort.substr(0,pos);
    // Resolve the server address and port
    iResult = getaddrinfo(ipAddress.c_str(), DEFAULT_PORT.c_str(), &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    u_long mode {1};
    ioctlsocket(ConnectSocket, FIONBIO, &mode);

    float window_width = 800;
    float window_height = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(window_width, window_height, "HanChat");

    client::InputTextBox input_text_box;

    input_text_box.box = { 0, window_height-50, window_width, 50 };

    client::ScrollBar scroll_bar {
            .track = {window_width - 20, 0, 20, window_height - 50},
            .thumb = {window_width - 20, 0, 20, 20}
    };

    std::list<client::MessageBox> message_boxes;

    SetTargetFPS(60);

    int scroll_position = 0;

    int maxMessagesInView {(int)(window_height - 50) / 50};

    std::thread t {rec, std::ref(ConnectSocket), std::ref(message_boxes), std::ref(input_text_box.box)};

    while (!WindowShouldClose()) {

        window_height = GetScreenHeight();
        window_width = GetScreenWidth();
        input_text_box.box = { 0, window_height-50, window_width, 50 };
        scroll_bar.track = {window_width - 20, 0, 20, window_height - 50};
        scroll_bar.thumb = {window_width - 20, 0, 20, 20};

        if (CheckCollisionPointRec(GetMousePosition(), input_text_box.box) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            input_text_box.is_active = true;
        } else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            input_text_box.is_active = false;
        }

        input_text_box.handleEvent(message_boxes, ConnectSocket);

        BeginDrawing();

        ClearBackground(PINK);

        DrawRectangleRec(input_text_box.box, BLUE);
        if (input_text_box.is_active) DrawRectangleLines((int)input_text_box.box.x, (int)input_text_box.box.y, (int)input_text_box.box.width, (int)input_text_box.box.height, RED);
        else DrawRectangleLines((int)input_text_box.box.x, (int)input_text_box.box.y, (int)input_text_box.box.width, (int)input_text_box.box.height, DARKGRAY);

        if (message_boxes.size() > maxMessagesInView) {
            if (GetMouseWheelMove() > 0) {
                scroll_position = std::min(scroll_position + 1, (int)message_boxes.size() - 1);
            } else if (GetMouseWheelMove() < 0) {
                scroll_position = std::max(scroll_position - 1, 0);
            }
        }


        DrawText(input_text_box.text.c_str(), input_text_box.box.x + 10, input_text_box.box.y + 10, 20, BLACK);

        auto it {message_boxes.begin()};
        int offset {0};
        while (it != message_boxes.end()) {
            it->box.y = input_text_box.box.y - 50 * (message_boxes.size() - offset - scroll_position);
            if (it->box.y < input_text_box.box.y) {
                DrawRectangleLines(it->box.x,
                                   it->box.y,
                                   it->box.width,
                                   it->box.height,
                                   BLACK);
                DrawText(it->text.c_str(),
                         it->box.x + 10,
                         it->box.y + 10,
                         20,
                         BLACK);
            }
            ++it;
            ++offset;
        }

        scroll_bar.update(scroll_position, message_boxes.size());
        scroll_bar.draw();

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    CloseWindow();

    on = false;
    t.join();
    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}