#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

class LogQueue
{
public:
    enum class State
    {
        WAIT,
        RUN,
        FINISH,
        ERR,
    };

public:
    std::thread::id id_;
    State state_;
};

class LogThread
{
public:
    enum class State
    {
        START,
        STOP,
    };

public:
    std::thread::id id_;
    std::string name_;
    State state_;
};

class Logger
{
private:
    std::mutex mtx_;
    std::vector<LogThread> logthreads;
    std::vector<LogQueue> logqueues;

public:
    static void createInstance();
    static Logger *getInstance();
    static void freeInstance();

    void addThread(std::thread::id id, std::string name);
    void updateThread(std::thread::id id, LogThread::State state);
    size_t addQueue(LogQueue::State state);
    void updateQueue(size_t idx, std::thread::id id, LogQueue::State state);

private:
    void print();
};

template <typename T>
class Queue
{
private:
    int32_t size_;
    std::deque<T> deque_;
    std::deque<size_t> deque_index_;

public:
    Queue(int32_t size) : size_(size), deque_(), deque_index_()
    {
    }

    bool put(T &&data)
    {
        if (size_ <= static_cast<int32_t>(deque_.size()))
        {
            (void)Logger::getInstance()->addQueue(LogQueue::State::ERR);
            return false;
        }
        deque_.emplace_back(std::move(data));
        size_t idx = Logger::getInstance()->addQueue(LogQueue::State::WAIT);
        deque_index_.emplace_back(idx);
        return true;
    }

    bool put(const T &data)
    {
        if (size_ <= static_cast<int32_t>(deque_.size()))
        {
            (void)Logger::getInstance()->addQueue(LogQueue::State::ERR);
            return false;
        }
        deque_.emplace_back(data);
        size_t idx = Logger::getInstance()->addQueue(LogQueue::State::WAIT);
        deque_index_.emplace_back(idx);
        return true;
    }

    bool get(T &data)
    {
        if (deque_.empty())
        {
            return false;
        }
        data = std::move(deque_.front());
        deque_.pop_front();
        return true;
    }

    size_t getIndex()
    {
        size_t idx = deque_index_.front();
        deque_index_.pop_front();
        return idx;
    }

    bool empty() const
    {
        return deque_.empty();
    }
};

class ThreadPool
{
private:
    Queue<std::function<void()>> queue_;
    std::vector<std::thread> threads_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool isRunning_;

public:
    ThreadPool(const int32_t threadCount, const int32_t queueSize);
    ~ThreadPool();
    bool add(std::function<void()> &&func);
    bool add(const std::function<void()> &func);

private:
    void main_task();
};
