#pragma once

#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <list>
#include <atomic>
#include <functional>

#ifdef WIN32
#include <WinSock2.h>
#else
typedef int32_t SOCKET;
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#endif

class Header
{
public:
    char magic_[4] = "SOC";
    int32_t size_ = 0;
};

class Socket
{
protected:
    int32_t logid_ = 0;

protected:
    std::mutex mtx_;
    SOCKET sock_ = INVALID_SOCKET;
    std::list<SOCKET> connectedSockets_;
    int32_t epfd_ = -1;

public:
    Socket(int32_t logid = 0);
    virtual ~Socket();
    SOCKET get() const;
    bool isConnected(SOCKET sock) const;
    bool do_create();
    void do_delete();
    int32_t do_send(const SOCKET sndSock, const char *sndData, const int32_t sndSize);
    int32_t do_recieve(SOCKET rcvSock, char **rcvData, int32_t *rcvSize);

private:
    int32_t recv_(SOCKET rcvSock, char *rcvData, int32_t rcvSize);
};

class ServerSocket : public Socket
{
public:
    ServerSocket(int32_t logid = 0);
    virtual ~ServerSocket() override;
    bool do_bind_listen(const std::string ipaddr, const uint16_t portNo);
    int32_t do_recieve_event(const std::function<void(SOCKET, const char *, const int32_t)> &func_recieve);
    SOCKET do_accept();
    void do_disconnect(SOCKET sock);
    void do_disconnect_all();
};

class ClientSocket : public Socket
{
public:
    ClientSocket(int32_t logid = 0);
    virtual ~ClientSocket() override;
    bool do_connect(const std::string ipaddr, const uint16_t portNo);
    void do_disconnect();
    int32_t do_send(const char *sndData, const int32_t sndSize);
    int32_t do_recieve(char **rcvData, int32_t *rcvSize);
    int32_t do_recieve_event(const std::function<void(SOCKET, const char *, const int32_t)> &func_recieve);
};

class Server
{
public:
    class Reciever
    {
    public:
        virtual ~Reciever();
        virtual void recieveData(const int32_t id, const char *data, const int32_t size) = 0;
    };

private:
    int32_t logid_ = 0;
    const std::string ipaddr_ = "127.0.0.1";
    const uint16_t portNo_ = 9876;

private:
    ServerSocket serverSock_;

    std::thread th_;
    std::mutex mtx_;
    Reciever *reciever_ = nullptr;
    bool isRunning_ = false;

public:
    Server(int32_t logid = 0);
    ~Server();
    void start(Reciever *reciever);
    void end();
    int32_t sendData(const int32_t id, const char *data, const int32_t size);

private:
    void task();
};

class Client
{
public:
    class Reciever
    {
    public:
        virtual ~Reciever();
        virtual void recieveData(const int32_t id, const char *data, const int32_t size) = 0;
    };

private:
    int32_t logid_ = 0;
    const std::string ipaddr_ = "127.0.0.1";
    const uint16_t portNo_ = 9876;

private:
    ClientSocket clientSock_;

    std::thread th_;
    std::mutex mtx_;
    Reciever *reciever_ = nullptr;
    bool isRunning_ = false;

public:
    Client(int32_t logid = 0);
    ~Client();
    void start(Reciever *reciever);
    void end();
    int32_t sendData(const char *data, const int32_t size);

private:
    void task();
};
