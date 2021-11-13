#include <chrono>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>

#include "MySocket.hpp"
#include "Logger.hpp"

//#define LOG_DEBUG(...)
#define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)

namespace
{
    static void wait_time(int32_t millisecond)
    {
        //LOG_DEBUG(".........\n");
#if 1
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecond));
#else
        const int32_t COUNT = 10;
        int32_t one_time = millisecond / COUNT;
        for (int32_t i = 0; i < COUNT; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(one_time));
        }
#endif
    }
}

class Wave
{
public:
    enum class Type
    {
        SIN,
        COS,
    };
    char *data_ = nullptr;
    int32_t size_ = 0;
    Type type_ = Type::SIN;

    Wave(Type type, const size_t size)
    {
        constexpr int32_t fs = 8000; //サンプリング周波数
        constexpr double A = 255.0;  // 振幅
        constexpr double f0 = 250.0; // 基本周波数
        type_ = type;
        size_ = static_cast<int32_t>(size);
        data_ = new char[size];
        for (int32_t n = 0; n < size_; n++)
        {
            if (type_ == Type::SIN)
            {
                data_[n] = static_cast<char>(A * sin(2.0 * M_PI * f0 * static_cast<double>(n) / fs));
            }
            else
            {
                data_[n] = static_cast<char>(A * cos(2.0 * M_PI * f0 * static_cast<double>(n) / fs));
            }
        }
    }

    ~Wave()
    {
        if (data_ != nullptr)
        {
            delete[] data_;
        }
    }
};

static Wave *g_sin_wave = nullptr;
static Wave *g_cos_wave = nullptr;

class Manager : public Server::Reciever
{
    int32_t logid_ = 0;
    Server server_;

public:
    Manager();
    virtual ~Manager() override;
    void start();
    void end();

public:
    void sendData(const int32_t id, const char *data, const int32_t size);

private:
    virtual void recieveData(const int32_t id, const char *data, const int32_t size) override;
};

Manager::Manager() : logid_(Logger::add("<Server>")), server_(logid_)
{
}

Manager::~Manager()
{
}

void Manager::start()
{
    server_.start(this);
}

void Manager::end()
{
    server_.end();
}

void Manager::sendData(const int32_t id, const char *data, const int32_t size)
{
    if ((id <= 0) || (data == nullptr))
    {
        return;
    }
    Logger::print(logid_, "sendData id:%d", id);
    Logger::print(logid_, " -> sz:%d", size);
    server_.sendData(id, data, size);
}

void Manager::recieveData(const int32_t id, const char *data, const int32_t size)
{
    if ((id <= 0) || (data == nullptr))
    {
        return;
    }
    int32_t result = std::memcmp(data, g_sin_wave->data_, static_cast<size_t>(size));
    Logger::print(logid_, "recvData id:%d", id);
    Logger::print(logid_, " -> sz:%d <%s>", size, (result == 0) ? "OK" : "NG");

    sendData(id, g_cos_wave->data_, g_cos_wave->size_);
}

class User : public Client::Reciever
{
    int32_t logid_ = 0;
    Client client_;

public:
    User();
    virtual ~User() override;
    void start();
    void end();

public:
    void sendData(const char *data, const int32_t size);

private:
    virtual void recieveData(const int32_t id, const char *data, const int32_t size) override;
};

User::User() : logid_(Logger::add("<Client>")), client_(logid_)
{
}

User::~User()
{
}

void User::start()
{
    client_.start(this);
}

void User::end()
{
    client_.end();
}

void User::sendData(const char *data, const int32_t size)
{
    if (data == nullptr)
    {
        return;
    }
    Logger::print(logid_, "sendData");
    Logger::print(logid_, " -> sz:%d", size);
    client_.sendData(data, size);
}

void User::recieveData(const int32_t id, const char *data, const int32_t size)
{
    if ((id <= 0) || (data == nullptr))
    {
        return;
    }
    int32_t result = std::memcmp(data, g_cos_wave->data_, static_cast<size_t>(size));
    Logger::print(logid_, "recvData id:%d", id);
    Logger::print(logid_, "sendData sz:%d <%s>", size, (result == 0) ? "OK" : "NG");
}

// サーバとクライアントが1対1で接続
// サーバとクライアントが1度だけデータの送受信を実施する
static void test1_1()
{
    Logger::init();
    {
        Manager manager;
        manager.start();
        wait_time(1000);
        User user;
        user.start();
        wait_time(1000);
        user.sendData(g_sin_wave->data_, g_sin_wave->size_);
        wait_time(1000);
    }
    Logger::deinit();
}

// サーバとクライアントが1対1で接続
// サーバとクライアントが大量にデータの送受信を繰り返す
static void test1_2()
{
    Logger::init();
    {
        Manager manager;
        manager.start();
        wait_time(1000);
        User user;
        user.start();
        wait_time(1000);
        for (int32_t i = 0; i < 10000; i++)
        {
            user.sendData(g_sin_wave->data_, g_sin_wave->size_);
        }
    }
    Logger::deinit();
}

// サーバとクライアントが1対1で接続
// クライアントが接続→送信→切断を繰り返す
static void test1_3()
{
    Logger::init();
    {
        Manager manager;
        manager.start();
        wait_time(1000);
        for (int32_t i = 0; i < 5; i++)
        {
            User user;
            user.start();
            wait_time(1000);
            user.sendData(g_sin_wave->data_, g_sin_wave->size_);
            wait_time(1000);
        }
    }
    Logger::deinit();
}

// サーバとクライアントが1対多で接続
// サーバとクライアントがデータの送受信を繰り返す
static void test2_1()
{
    Logger::init();
    {
        Manager manager;
        manager.start();
        {
            User user1;
            user1.start();
            User user2;
            user2.start();
            User user3;
            user3.start();
            User user4;
            user4.start();
            User user5;
            user5.start();
            wait_time(1000);
            for (int32_t i = 0; i < 100; i++)
            {
                user1.sendData(g_sin_wave->data_, g_sin_wave->size_);
                user2.sendData(g_sin_wave->data_, g_sin_wave->size_);
                user3.sendData(g_sin_wave->data_, g_sin_wave->size_);
                user4.sendData(g_sin_wave->data_, g_sin_wave->size_);
                user5.sendData(g_sin_wave->data_, g_sin_wave->size_);
            }
            wait_time(1000);
        }
    }
    Logger::deinit();
}

// サーバとクライアントが1対1で接続
// クライアントが接続→送信→切断を繰り返す
static void test3_1()
{
    Logger::init();
    {
        User user;
        user.start();
        wait_time(1000);
        for (int32_t i = 0; i < 5; i++)
        {
            Manager manager;
            manager.start();
            wait_time(1000);
            user.sendData(g_sin_wave->data_, g_sin_wave->size_);
            wait_time(1000);
        }
    }
    Logger::deinit();
}

// サーバとクライアントが1対1で接続
// サーバとの接続が絶たれているときにデータ送信
static void test3_2()
{
    Logger::init();
    {
        User user;
        user.start();

        // 接続前に送信
        user.sendData(g_sin_wave->data_, g_sin_wave->size_);
        wait_time(1000);

        {
            // 接続して送信
            Manager manager;
            manager.start();
            wait_time(1000);
            user.sendData(g_sin_wave->data_, g_sin_wave->size_);
            wait_time(1000);
        }
        wait_time(1000);

        // 接続後、切断してから送信
        user.sendData(g_sin_wave->data_, g_sin_wave->size_);
        wait_time(1000);
    }
    Logger::deinit();
}

int32_t main()
{

    g_sin_wave = new Wave(Wave::Type::SIN, 2 * 1024 * 1024);
    g_cos_wave = new Wave(Wave::Type::COS, 3 * 1024 * 1024);

    LOG_DEBUG("\n----------- test1_1 START -----------\n");
    test1_1();
    LOG_DEBUG("\n----------- test1_1 END -----------\n");

    LOG_DEBUG("\n----------- test1_2 START -----------\n");
    test1_2();
    LOG_DEBUG("\n----------- test1_2 END -----------\n");

    LOG_DEBUG("\n----------- test1_3 START -----------\n");
    test1_3();
    LOG_DEBUG("\n----------- test1_3 END -----------\n");

    LOG_DEBUG("\n----------- test2_1 START -----------\n");
    test2_1();
    LOG_DEBUG("\n----------- test2_1 END -----------\n");

    LOG_DEBUG("\n----------- test3_1 START -----------\n");
    test3_1();
    LOG_DEBUG("\n----------- test3_1 END -----------\n");

    LOG_DEBUG("\n----------- test3_2 START -----------\n");
    test3_2();
    LOG_DEBUG("\n----------- test3_2 END -----------\n");

    delete g_sin_wave;
    delete g_cos_wave;

    return 0;
}
