#include "MySocket.hpp"

#include <cstdio>
#include <chrono>

#include <WS2tcpip.h>

//#define LOG_DEBUG(...)
#define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)

Socket::Socket(std::string debug) : debug_(debug)
{
}

Socket::~Socket()
{
    th_.join();

    auto itr = clientSockets_.begin();
    while (itr != clientSockets_.end())
    {
        SOCKET rcvSock = *itr;
        closesocket(rcvSock);
        itr = clientSockets_.erase(itr);
    }

    closesocket(sock_);
}

void Socket::start()
{
    int32_t ret = 0;

    // winsockの初期化
    WSAData wsaData;
    ret = WSAStartup(MAKEWORD(2, 0), &wsaData);
    if (ret != 0)
    {
        LOG_ERROR("[%s] !!!ERROR!!! WSAStartup() ret:%d\n", debug_.c_str(), ret);
        return;
    }
    //LOG_DEBUG("[%s] WSAStartup() ret:%d\n", debug_.c_str(), ret);

    std::thread th(&Socket::task, this);
    th_.swap(th);
}

void Socket::sendData(const SOCKET sock, const char *data, const int32_t size)
{
    Socket::Header header;
    header.size_ = size;
    int32_t headerSize = static_cast<int32_t>(sizeof(header));
    int32_t newSize = size + headerSize;
    char *newData = new char[newSize];
    std::memcpy(newData, &header, headerSize);
    std::memcpy(newData + headerSize, data, size);
    int32_t ret = send(sock, newData, newSize, 0);
    if (ret == -1)
    {
        LOG_ERROR("[%s] !!!ERROR!!! send() size:%d err:%d\n", debug_.c_str(), newSize, WSAGetLastError());
    }
    LOG_DEBUG("[%s] send() size:%d\n", debug_.c_str(), newSize);
}

bool Socket::do_create_socket()
{
    // ソケットを作成する
    // AF_INET: IPv4 SOCK_STREAM:TCP
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET)
    {
        LOG_ERROR("[%s] !!!ERROR!!! socket() sock_:0x%llx err:%d\n", debug_.c_str(), sock_, WSAGetLastError());
        return false;
    }
    LOG_DEBUG("[%s] socket() sock_:0x%llx\n", debug_.c_str(), sock_);
    return true;
}

bool Socket::do_bind_listen()
{
    int32_t ret = 0;

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(portNo_);
    sa.sin_addr.S_un.S_addr = INADDR_ANY;

    // SO_REUSEADDRを有効にする
    // TIME_WAIT状態のポートが存在していてもbindができるようにする
    int32_t yes = 1;
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));

    // リッスンソケットをバインド
    ret = ::bind(sock_, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa));
    if (ret != 0)
    {
        LOG_ERROR("[%s] !!!ERROR!!! bind() sock_:0x%llx err:%d\n", debug_.c_str(), sock_, WSAGetLastError());
        closesocket(sock_);
        return false;
    }
    LOG_DEBUG("[%s] bind() sock_:0x%llx\n", debug_.c_str(), sock_);

    // リッスン開始
    ret = ::listen(sock_, 0);
    if (ret != 0)
    {
        LOG_ERROR("[%s] !!!ERROR!!! listen() sock_:0x%llx err:%d\n", debug_.c_str(), sock_, WSAGetLastError());
        closesocket(sock_);
        return false;
    }
    LOG_DEBUG("[%s] listen() sock_:0x%llx\n", debug_.c_str(), sock_);
    return true;
}

bool Socket::do_connect()
{
    int32_t ret = 0;

    struct sockaddr_in sa_server;
    sa_server.sin_family = AF_INET;
    sa_server.sin_port = htons(portNo_);
    inet_pton(sa_server.sin_family, ipaddr_.c_str(), &sa_server.sin_addr.S_un.S_addr);

    // 接続
    //LOG_DEBUG("[%s] connect() waiting...\n", debug_.c_str());
    ret = connect(sock_, reinterpret_cast<struct sockaddr *>(&sa_server), sizeof(sa_server));
    if (ret != 0)
    {
        //LOG_ERROR("[%s] !!!ERROR!!! connect() sock_:0x%llx err:%d\n", debug_.c_str(), sock_, WSAGetLastError());
        return false;
    }
    LOG_DEBUG("[%s] connect() sock_:0x%llx\n", debug_.c_str(), sock_);
    return true;
}

int32_t Socket::do_recieve(SOCKET rcvSock)
{
    int32_t ret = 0;
    Socket::Header header;
    ret = recv_(rcvSock, reinterpret_cast<char *>(&header), sizeof(header));
    if (ret == SOCKET_ERROR)
    {
        // エラー
        LOG_ERROR("[%s] !!!ERROR!!! recv_() header SOCKET_ERROR err:%d\n", debug_.c_str(), WSAGetLastError());
        return ret;
    }
    if (ret == 0)
    {
        // 接続が切れた
        LOG_DEBUG("[%s] recv_() header disconnect\n", debug_.c_str());
        return ret;
    }
    //LOG_DEBUG("[%s] recv_() header magic:%s size:%d\n", debug_.c_str(), header.magic_, header.size_);

    if (std::memcmp(header.magic_, "SOC", 3) != 0)
    {
        LOG_ERROR("[%s] !!!ERROR!!! recv_() header magic invalid\n", debug_.c_str());
        return SOCKET_ERROR;
    }

    int32_t rcvSize = header.size_;
    char *rcvData = new char[rcvSize];
    ret = recv_(rcvSock, rcvData, rcvSize);
    if (ret == SOCKET_ERROR)
    {
        // エラー
        LOG_ERROR("[%s] !!!ERROR!!! recv_() data SOCKET_ERROR err:%d\n", debug_.c_str(), WSAGetLastError());
        return ret;
    }
    if (ret == 0)
    {
        // 接続が切れた
        LOG_ERROR("[%s] !!!ERROR!!! recv_() data disconnect err:%d\n", debug_.c_str(), WSAGetLastError());
        return ret;
    }
    //LOG_DEBUG("[%s] recv_() data size:%d\n", debug_.c_str(), ret);
    recieveData(static_cast<int32_t>(rcvSock), rcvData, rcvSize);
    delete[] rcvData;
    return ret;
}

int32_t Socket::recv_(SOCKET rcvSock, char *rcvData, int32_t rcvSize)
{
    int32_t remainSize = rcvSize;
    int32_t recievedSize = 0;
    while (remainSize > 0)
    {
        int32_t sz = ::recv(rcvSock, rcvData + recievedSize, remainSize, 0);
        LOG_DEBUG("[%s] recv() rcvSize:%d remainSize:%d sz:%d\n", debug_.c_str(), rcvSize, remainSize, sz);
        if (sz == SOCKET_ERROR)
        {
            if (errno == EAGAIN)
            {
                // EAGAINは処理続行
                continue;
            }
            return sz;
        }
        else if (sz == 0)
        {
            return sz;
        }
        else
        {
            recievedSize += sz;
            remainSize -= sz;
        }
    }
    return recievedSize;
}

Server::Server() : Socket("Server")
{
}

Server::~Server()
{
    isRunning_ = false;
}

void Server::sendData(const int32_t id, const char *data, const int32_t size)
{
    Socket::sendData(static_cast<SOCKET>(id), data, size);
}

int32_t Server::do_select(fd_set *fds)
{
    FD_ZERO(fds);

    // リッスンソケットをselectで待つソケットとして登録する
    FD_SET(sock_, fds);

    // クライアントソケットをselectで待つソケットとして登録する
    // また、select()の第1引数としてmaxfdを計算する
    // maxfdは、WinSockでは無視されるため0とする
    int32_t maxfd = 0;
    for (auto itr = clientSockets_.begin(); itr != clientSockets_.end(); ++itr)
    {
        FD_SET(*itr, fds);
    }

    // 5秒でタイムアウトさせる
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    // fdsに設定されたソケットが読み込み可能になるまで待つ
    //LOG_DEBUG("[Server] select() waiting...\n");
    return select(maxfd, fds, nullptr, nullptr, &tv);
}

void Server::do_accept()
{
    // クライアントを受付
    struct sockaddr_in sa_client;
    socklen_t len = sizeof(sa_client);
    SOCKET client = accept(sock_, reinterpret_cast<struct sockaddr *>(&sa_client), &len);
    if (client == INVALID_SOCKET)
    {
        LOG_ERROR("[Server] !!!ERROR!!! accept() err:%d\n", WSAGetLastError());
        return;
    }
    char ip[32];
    memset(ip, 0, sizeof(ip));
    inet_ntop(sa_client.sin_family, &sa_client.sin_addr, ip, sizeof(ip));
    LOG_DEBUG("[Server] accept() client:0x%llx ip:%s port:%d\n", client, ip, ntohs(sa_client.sin_port));
    // ソケットリストに登録する
    clientSockets_.push_back(client);
}

void Server::task()
{
    //LOG_DEBUG("[Server] task() start\n");
    int32_t ret = 0;

    // ソケット作成
    if (!do_create_socket())
    {
        return;
    }

    // バインド/リッスン開始
    if (!do_bind_listen())
    {
        return;
    }

    while (isRunning_)
    {
        fd_set fds;
        ret = do_select(&fds);
        if (ret == SOCKET_ERROR)
        {
            LOG_ERROR("[Server] !!!ERROR!!! select() SOCKET_ERROR err:%d\n", WSAGetLastError());
            continue;
        }
        if (ret == 0)
        {
            LOG_DEBUG("[Server] select() time limit expired\n");
            continue;
        }

        if (FD_ISSET(sock_, &fds))
        {
            // リッスンソケットに変化あり
            // クライアントを受付
            do_accept();
        }
        else
        {
            // クライアントソケットに変化あり
            auto itr = clientSockets_.begin();
            while (itr != clientSockets_.end())
            {
                SOCKET rcvSock = *itr;

                // 変化がないソケットについては何もしない
                if (!FD_ISSET(rcvSock, &fds))
                {
                    ++itr;
                    continue;
                }

                ret = do_recieve(rcvSock);
                if (ret == SOCKET_ERROR)
                {
                    ++itr;
                    continue;
                }
                if (ret == 0)
                {
                    // 接続が切れたため、クライアントソケットから削除する
                    closesocket(rcvSock);
                    itr = clientSockets_.erase(itr);
                    continue;
                }

                ++itr;
            }
        }
    }
    //LOG_DEBUG("[Server] task() end\n");
}

Client::Client() : Socket("Client")
{
}

Client::~Client()
{
    isRunning_ = false;
}

void Client::sendData(const char *data, const int32_t size)
{
    Socket::sendData(sock_, data, size);
}

int32_t Client::do_select(fd_set *fds)
{
    FD_ZERO(fds);

    // ソケットをselectで待つソケットとして登録する
    FD_SET(sock_, fds);

    int32_t maxfd = 0;

    // 5秒でタイムアウトさせる
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    // fdsに設定されたソケットが読み込み可能になるまで待つ
    //LOG_DEBUG("[Client] select() waiting...\n");
    return select(maxfd, fds, nullptr, nullptr, &tv);
}

void Client::task()
{
    //LOG_DEBUG("[Client] task() start\n");
    int32_t ret = 0;

    while (isRunning_)
    {
        // ソケット作成
        if (!do_create_socket())
        {
            // リトライ
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return;
        }

        // 接続
        if (!do_connect())
        {
            // リトライ
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        while (isRunning_)
        {
            fd_set fds;
            ret = do_select(&fds);
            if (ret == SOCKET_ERROR)
            {
                LOG_ERROR("[Client] !!!ERROR!!! select() SOCKET_ERROR err:%d\n", WSAGetLastError());
                continue;
            }
            if (ret == 0)
            {
                LOG_DEBUG("[Client] select() time limit expired\n");
                continue;
            }

            if (FD_ISSET(sock_, &fds))
            {
                // ソケットに変化あり

                ret = do_recieve(sock_);
                if (ret == SOCKET_ERROR)
                {
                    continue;
                }
                if (ret == 0)
                {
                    // 接続が切れたため、再接続させる
                    closesocket(sock_);
                    sock_ = 0;
                    break;
                }
            }
        }
    }

    //LOG_DEBUG("[Client] task() end\n");
}
