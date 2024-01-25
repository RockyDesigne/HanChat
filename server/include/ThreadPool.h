//
// Created by HORIA on 25.01.2024.
//

#ifndef SERVER_THREADPOOL_H
#define SERVER_THREADPOOL_H

#include <functional>
#include <thread>
#include <condition_variable>
#include <queue>

class ThreadPool {
public:
    class Task {
    public:
        explicit Task(std::function<void()> func) : m_func(std::move(func)) {}
        Task()=default;
        void execute() { m_func(); }

    private:
        std::function<void()> m_func;
    };

    explicit ThreadPool(size_t numThreads);

    ~ThreadPool();

    void enqueue(Task task);

private:
    std::vector<std::thread> m_threads;
    std::condition_variable m_eventVar;
    std::mutex m_eventMutex;
    bool m_stopping {false};
    std::queue<Task> m_tasks;

    void start(std::size_t numThreads);

    void stop();

};


#endif //SERVER_THREADPOOL_H
