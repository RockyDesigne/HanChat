//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_MY_EXCEPTION_H
#define SERVER_MY_EXCEPTION_H

#include <exception>
#include <string_view>
#include <string>

class Exception : public std::exception {
public:
    explicit Exception(std::string_view msg) : m_msg {msg} {}
    //not to self
    //noexcept keyword indicates the func does not throw an exception
    //if you throw an except, the program will terminate with std::terminate()
    [[nodiscard]] const char* what() const noexcept override {return m_msg.data();}
private:
    std::string m_msg {};
};


#endif //SERVER_MY_EXCEPTION_H
