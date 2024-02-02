//
// Created by HORIA on 31.01.2024.
//

#ifndef CLIENT_GUI_H
#define CLIENT_GUI_H
#if defined(_WIN32)
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif
#define WIN32_LEAN_AND_MEAN
#if defined(_WIN32)           // raylib uses these names as function parameters
#undef near
#undef far
#endif
#include <raylib.h>
#include <string>
#include <list>

static int MAXM_MESSAGES_NUM = 1000;

namespace client {
    struct MessageBox {
        Rectangle box;
        std::string text;
    };
    struct InputTextBox {
        Rectangle box;
        std::string text;
        bool is_active=false;
        bool is_entered=false;
        void handleEvent(std::list<MessageBox>& message_boxes, SOCKET &sock) {
            if (is_active) {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125)) {
                        text += (char)key;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    if (text.length() > 0) {
                        text.pop_back();
                    }
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    if (message_boxes.size() > MAXM_MESSAGES_NUM) {
                        message_boxes.pop_front();
                    }
                    MessageBox message_box {box.x + 50, box.y - 50 * (message_boxes.size() + 1), box.width - 50, 50, text};
                    message_boxes.push_back(message_box);
                    send(sock, text.c_str(), text.length(), 0);
                    text.clear();
                }
            } else {
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }
        }
    };
    struct ScrollBar {
        Rectangle track;
        Rectangle thumb;

        void draw() {
            DrawRectangleRec(track, GRAY);
            DrawRectangleRec(thumb, DARKGRAY);
        }

        void update(int scroll_position, int total_messages) {
            thumb.height = std::max(track.height / total_messages, 20.0f);
            thumb.y = track.y + scroll_position * (track.height - thumb.height) / (total_messages - 1);
        }
    };
}

#endif //CLIENT_GUI_H
