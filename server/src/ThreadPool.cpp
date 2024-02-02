//
// Created by HORIA on 25.01.2024.
//

#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) {
    start(numThreads);
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::enqueue(ThreadPool::Task task) {
    {
        std::unique_lock<std::mutex> lock(m_eventMutex); // ensures thread safety of the queue
        m_tasks.emplace(std::move(task));
    }
    m_eventVar.notify_one(); //using it to synchronize threads
}

void ThreadPool::start(std::size_t numThreads) {
    for (std::size_t i {}; i < numThreads; ++i) {
        m_threads.emplace_back([=, this] {
            while (true) {
                Task task;
                {
                    std::unique_lock<std::mutex> lock {m_eventMutex};
                    m_eventVar.wait(lock,[=, this]{return (m_stopping || !m_tasks.empty());});
                    if (m_stopping && m_tasks.empty()) {
                        break;
                    }
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }
                task.execute();
            }
        });
    }
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(m_eventMutex);
        m_stopping = true;
    }
    m_eventVar.notify_all();
    for (auto &thread : m_threads)
        thread.join();
}
