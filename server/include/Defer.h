//
// Created by HORIA on 23.01.2024.
//

#ifndef SERVER_DEFER_H
#define SERVER_DEFER_H

#include <utility>

namespace dfr {
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
}
#endif //SERVER_DEFER_H
