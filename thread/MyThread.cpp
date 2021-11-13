#include "MyThread.hpp"

#include <iostream>
#include <string>
#include <cassert>

#include <cstdlib>

#ifdef WIN32
#include <windows.h>
#include <processthreadsapi.h>
#endif

//#define LOG_DEBUG(...)
#define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)

#if 0
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define TAG "thread_test"
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#else
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define LOG_INFO(...) fprintf(stdout, __VA_ARGS__)
#endif
#endif

namespace
{
    Logger *logger = nullptr;
}

void Logger::createInstance()
{
    if (logger == nullptr)
    {
        logger = new Logger;
    }
}
Logger *Logger::getInstance()
{
    return logger;
}
void Logger::freeInstance()
{
    if (logger != nullptr)
    {
        delete logger;
        logger = nullptr;
    }
}

void Logger::addThread(std::thread::id id, std::string name)
{
    std::lock_guard<std::mutex> lock(mtx_);
    LogThread log;
    log.id_ = id;
    log.name_ = name;
    log.state_ = LogThread::State::START;
    logthreads.emplace_back(log);
    print();
}

void Logger::updateThread(std::thread::id id, LogThread::State state)
{
    for (size_t i = 0; i < logthreads.size(); i++)
    {
        if (logthreads[i].id_ == id)
        {
            logthreads[i].state_ = state;
            break;
        }
    }
    print();
}

size_t Logger::addQueue(LogQueue::State state)
{
    std::lock_guard<std::mutex> lock(mtx_);
    LogQueue log;
    log.state_ = state;
    logqueues.emplace_back(log);
    print();
    return logqueues.size() - 1;
}

void Logger::updateQueue(size_t idx, std::thread::id id, LogQueue::State state)
{
    if (idx >= logqueues.size())
    {
        return;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    logqueues[idx].id_ = id;
    logqueues[idx].state_ = state;
    print();
}

void Logger::print()
{
    LOG_DEBUG("\x1B[2J\x1B[H");

    std::string head;
    head += "\t";
    for (size_t i = 0; i < logthreads.size(); i++)
    {
        head += "\t" + logthreads[i].name_;
    }
    LOG_DEBUG("%s\n", head.c_str());

    std::string head2;
    head2 += "\t";
    for (size_t i = 0; i < logthreads.size(); i++)
    {
        if (logthreads[i].state_ == LogThread::State::START)
        {
            head2 += "\tSTART";
        }
        else
        {
            head2 += "\tSTOP";
        }
    }
    LOG_DEBUG("%s\n", head2.c_str());

    for (size_t i = 0; i < logqueues.size(); i++)
    {
        std::string row;
        row += std::to_string(i) + "\t";
        if ((logqueues[i].state_ != LogQueue::State::WAIT) && (logqueues[i].state_ != LogQueue::State::ERR))
        {
            for (size_t t = 0; t < logthreads.size(); t++)
            {
                row += "\t";
                if (logqueues[i].id_ == logthreads[t].id_)
                {
                    break;
                }
            }
        }
        switch (logqueues[i].state_)
        {
        case LogQueue::State::WAIT:
            row += "WAIT";
            break;
        case LogQueue::State::RUN:
            row += "RUN";
            break;
        case LogQueue::State::FINISH:
            row += "FIN";
            break;
        case LogQueue::State::ERR:
            row += "ERR";
            break;
        default:
            break;
        }
        LOG_DEBUG("%s\n", row.c_str());
    }
}

ThreadPool::ThreadPool(const int32_t threadCount, const int32_t queueSize) : queue_(queueSize), isRunning_(true)
{
    Logger::createInstance();
    for (size_t i = 0; i < static_cast<size_t>(threadCount); i++)
    {
        threads_.emplace_back(std::thread(&ThreadPool::main_task, this));
        Logger::getInstance()->addThread(threads_[i].get_id(), "TH" + std::to_string(i));
#ifdef WIN32
        std::wstring name = L"WokerThread" + std::to_wstring(i);
        SetThreadDescription(threads_.at(i).native_handle(), name.c_str());
#endif
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isRunning_ = false;
    }
    cv_.notify_all();
    const size_t size = threads_.size();
    for (size_t i = 0; i < size; i++)
    {
        threads_.at(i).join();
    }
    Logger::freeInstance();
}

bool ThreadPool::add(std::function<void()> &&func)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool result = queue_.put(std::move(func));
        if (!result)
        {
            return false;
        }
        cv_.notify_all();
        return true;
    }
}

bool ThreadPool::add(const std::function<void()> &func)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool result = queue_.put(func);
        if (!result)
        {
            return false;
        }
        cv_.notify_all();
        return true;
    }
}

void ThreadPool::main_task()
{
    while (true)
    {
        std::function<void()> func;
        size_t idx = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (queue_.empty())
            {
                if (!isRunning_)
                {
                    Logger::getInstance()->updateThread(std::this_thread::get_id(), LogThread::State::STOP);
                    return;
                }
                cv_.wait(lock);
            }
            const bool result = queue_.get(func);
            idx = queue_.getIndex();
            assert(result);
        }
        Logger::getInstance()->updateQueue(idx, std::this_thread::get_id(), LogQueue::State::RUN);
        func();
        Logger::getInstance()->updateQueue(idx, std::this_thread::get_id(), LogQueue::State::FINISH);
    }
}
