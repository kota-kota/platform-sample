#include "Logger.hpp"
#include <stdarg.h>
#include <mutex>

#define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)

namespace
{
    static Logger *instance_ = nullptr;
    //std::mutex mtx_;

    static constexpr int32_t TAB_NUM = 3;

    std::string tabs(int32_t id)
    {
        std::string tabs;
        for (int32_t i = 0; i < id * TAB_NUM; i++)
        {
            tabs += "\t";
        }
        return tabs;
    }
}

Log::Log(int32_t id, std::string header) : id_(id), header_(header)
{
    LOG_DEBUG("%s%s\n", tabs(id_).c_str(), header_.c_str());
}

Logger::Logger()
{
}

Logger::~Logger()
{
}

void Logger::init()
{
    if (instance_ == nullptr)
    {
        instance_ = new Logger();
    }
}

void Logger::deinit()
{
    if (instance_ != nullptr)
    {
        delete instance_;
        instance_ = nullptr;
    }
}

int32_t Logger::add(std::string header)
{
    return instance_->add_in(header);
}
int32_t Logger::add_in(std::string header)
{
    int32_t idx = static_cast<int32_t>(logs_.size());
    logs_.emplace_back(idx, header);
    return idx;
}

void Logger::print(int32_t logid, char *fmt, ...)
{
    char wk[1024];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(&wk[0], fmt, ap);
    va_end(ap);
    instance_->print_(logid, wk);
}
void Logger::print_(int32_t logid, char *logtext)
{
    int32_t id = (logid <= 0) ? 0 : logid;
    LOG_DEBUG("%s%s\n", tabs(id).c_str(), logtext);
}
