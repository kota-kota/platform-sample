#pragma once

#include <cstdint>
#include <vector>
#include <string>

class Log
{
    int32_t id_;
    std::string header_;
    std::vector<std::string> texts_;

public:
    Log(int32_t id, std::string header);
};

class Logger
{
    std::vector<Log> logs_;

private:
    Logger();
    ~Logger();

public:
    static void init();
    static void deinit();
    static int32_t add(std::string header);
    int32_t add_in(std::string header);
    static void print(int32_t logid, char *fmt, ...);
    void print_(int32_t logid, char *logtext);
};
