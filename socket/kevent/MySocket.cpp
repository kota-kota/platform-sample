#include "MySocket.hpp"
#include "Logger.hpp"

#include <cstdio>
#include <cstring>
#include <chrono>

#include <sys/socket.h> // socket(), setsockopt(), bind()
#include <netinet/in.h> // sockaddr_in, htons()
#include <unistd.h>     // close()
#include <arpa/inet.h>  // inet_pton()
#include <sys/event.h>  // kevent系

#if 1
#define LOG_DEBUG(...)
//#define LOG_ERROR(...)
#else
#define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#endif

Socket::Socket(int32_t logid) : logid_(logid)
{
}

Socket::~Socket()
{
    do_delete();
}

SOCKET Socket::get() const
{
    return sock_;
}

bool Socket::isConnected(SOCKET sock) const
{
    if (sock == INVALID_SOCKET)
    {
        return false;
    }

    auto itr = connectedSockets_.begin();
    while (itr != connectedSockets_.end())
    {
        if (*itr == sock)
        {
            break;
        }
        ++itr;
    }
    if (itr == connectedSockets_.end())
    {
        return false;
    }

    return true;
}

bool Socket::do_create()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (sock_ != INVALID_SOCKET)
    {
        // ソケット作成済みのため何もしない
        return true;
    }

    // ソケットを作成する
    // AF_INET: IPv4 SOCK_STREAM:TCP
    sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET)
    {
        Logger::print(logid_, "ERR! create sock err:%d", errno);
        return false;
    }
    Logger::print(logid_, "create sock:0x%x", sock_);

    // カーネルイベントキューを生成する
    epfd_ = ::kqueue();
    if (epfd_ == -1)
    {
        Logger::print(logid_, "ERR! create epfd err:%d", errno);
        return false;
    }
    Logger::print(logid_, "create epfd:%d", epfd_);
    return true;
}

void Socket::do_delete()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (epfd_ != -1)
    {
        Logger::print(logid_, "close epfd:%d", epfd_);
        ::close(epfd_);
        epfd_ = -1;
    }
    if (sock_ != INVALID_SOCKET)
    {
        Logger::print(logid_, "close sock:0x%x", sock_);
        ::close(sock_);
        sock_ = INVALID_SOCKET;
    }
}

int32_t Socket::do_send(const SOCKET sndSock, const char *sndData, const int32_t sndSize)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (!isConnected(sndSock))
    {
        Logger::print(logid_, "ERR! send disconnect sock:0x%x", sndSock);
        return -1;
    }

    Header header;
    header.size_ = sndSize;
    size_t headerSize = sizeof(header);
    size_t newSize = static_cast<size_t>(sndSize) + headerSize;
    char *newData = new char[newSize];
    std::memcpy(newData, &header, headerSize);
    std::memcpy(newData + headerSize, sndData, static_cast<size_t>(sndSize));

    // 送信
    // SIGPIPEを発生させないために、flagsにMSG_NOSIGNALを設定する
    // 参考:https://daeudaeu.com/sigpipe/
    ssize_t ret = ::send(sndSock, newData, newSize, MSG_NOSIGNAL);
    if (ret == -1)
    {
        Logger::print(logid_, "ERR! send sock:0x%x err:%d", sndSock, errno);
        delete[] newData;
        return -1;
    }
    Logger::print(logid_, "send sock:0x%x", sndSock);
    Logger::print(logid_, " -> size:%zu", newSize);
    delete[] newData;
    return 0;
}

int32_t Socket::do_recieve(SOCKET rcvSock, char **rcvData, int32_t *rcvSize)
{
    int32_t ret = 0;

    Header rcvHeader;
    ret = recv_(rcvSock, reinterpret_cast<char *>(&rcvHeader), sizeof(rcvHeader));
    if (ret == SOCKET_ERROR)
    {
        // エラー
        Logger::print(logid_, "ERR! recv head sock:0x%x err:%d", rcvSock, errno);
        return ret;
    }
    if (ret == 0)
    {
        // 接続が切れた
        Logger::print(logid_, "recv head disconnect sock:0x%x", rcvSock);
        return ret;
    }
    Logger::print(logid_, "recv head sock:0x%x", rcvSock);
    Logger::print(logid_, " -> %s sz:%d", rcvHeader.magic_, rcvHeader.size_);

    if (std::memcmp(rcvHeader.magic_, "SOC", 3) != 0)
    {
        Logger::print(logid_, "ERR! recv head invalid");
        return SOCKET_ERROR;
    }

    char *data = new char[static_cast<size_t>(rcvHeader.size_)];
    ret = recv_(rcvSock, data, rcvHeader.size_);
    if (ret == SOCKET_ERROR)
    {
        // エラー
        Logger::print(logid_, "ERR! recv data sock:0x%x err:%d", rcvSock, errno);
        delete[] data;
        return ret;
    }
    if (ret == 0)
    {
        // 接続が切れた
        Logger::print(logid_, "recv data disconnect sock:0x%x", rcvSock);
        delete[] data;
        return ret;
    }
    Logger::print(logid_, "recv data sock:0x%x", rcvSock);
    Logger::print(logid_, " -> sz:%d", ret);

    *rcvData = data;
    *rcvSize = rcvHeader.size_;

    return ret;
}

int32_t Socket::recv_(SOCKET rcvSock, char *rcvData, int32_t rcvSize)
{
    int32_t remainSize = rcvSize;
    int32_t recievedSize = 0;
    while (remainSize > 0)
    {
        ssize_t sz = ::recv(rcvSock, rcvData + recievedSize, static_cast<size_t>(remainSize), 0);
        if (sz == SOCKET_ERROR)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                // EAGAIN/EWOULDBLOCKは処理続行
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            return static_cast<int32_t>(sz);
        }
        else if (sz == 0)
        {
            return static_cast<int32_t>(sz);
        }
        else
        {
            recievedSize += static_cast<int32_t>(sz);
            remainSize -= static_cast<int32_t>(sz);
        }
    }
    return recievedSize;
}

ServerSocket::ServerSocket(int32_t logid) : Socket(logid)
{
}

ServerSocket::~ServerSocket()
{
    do_disconnect_all();
}

bool ServerSocket::do_bind_listen(const std::string ipaddr, const uint16_t portNo)
{
    LOG_DEBUG("[Server\t](%4d) ipaddr:%s portNo:%d\n", __LINE__, ipaddr.c_str(), portNo);
    int32_t ret = 0;

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(portNo);
    //sa.sin_addr.s_addr = INADDR_ANY;
    inet_pton(sa.sin_family, ipaddr.c_str(), &sa.sin_addr.s_addr);

    // SO_REUSEADDRを有効にする
    // TIME_WAIT状態のポートが存在していてもbindができるようにする
    int32_t yes = 1;
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));

    // リッスンソケットをバインド
    ret = ::bind(sock_, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa));
    if (ret != 0)
    {
        Logger::print(logid_, "ERR! bind sock:0x%x err:%d", sock_, errno);
        return false;
    }
    Logger::print(logid_, "bind sock:0x%x", sock_);
    Logger::print(logid_, " -> %s:%d", ipaddr.c_str(), portNo);

    // リッスン開始
    ret = ::listen(sock_, 0);
    if (ret != 0)
    {
        Logger::print(logid_, "ERR! listen sock:0x%x err:%d", sock_, errno);
        return false;
    }
    Logger::print(logid_, "listen sock:0x%x", sock_);

    return true;
}

int32_t ServerSocket::do_recieve_event(const std::function<void(SOCKET, const char *, const int32_t)> &func_recieve)
{
    int32_t result = 0;

    // ソケットをkqueueの監視対象に加える
	struct kevent kev;
	//EV_SET(&kev, sock_, EVFILT_VNODE, EV_ADD, NOTE_DELETE | NOTE_READ, 0, NULL);
	EV_SET(&kev, sock_, EVFILT_READ, EV_ADD, 0, 0, NULL);
	int32_t ret = ::kevent(epfd_, &kev, 1, NULL, 0, NULL);
    if (ret == -1)
    {
        Logger::print(logid_, "ERR! kev_add 1 epfd err:%d", errno);
        return -1;
    }

	static constexpr int32_t MAX_EVENTS = 16;
	struct kevent events[MAX_EVENTS];
	struct timespec timeout;
	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;
	int32_t nfds = ::kevent(epfd_, NULL, 0, events, MAX_EVENTS, &timeout);

    // readyとなったfd数分ループ
    for (int32_t n = 0; n < nfds; n++)
    {
        SOCKET client = static_cast<SOCKET>(events[n].ident);
        if (client == sock_)
        {
            SOCKET s = do_accept();
            if (s == INVALID_SOCKET)
            {
                // 受付エラー
                continue;
            }
        }
        else
        {
            // クライアントからデータ受信
            if (events[n].flags & EV_EOF)
            {
                // 接続が切れたため、クライアントソケットから削除する
                Logger::print(logid_, "disconnect client:0x%x", client);
                do_disconnect(client);
            }
            //else if (events[n].fflags & NOTE_READ)
			else
            {
                char *data = nullptr;
                int32_t size = 0;
                ret = do_recieve(client, &data, &size);
                if (ret > 0)
                {
                    func_recieve(client, data, size);
                }
                if (data != nullptr)
                {
                    delete[] data;
                }
            }
#if 0
            else
            {
                Logger::print(logid_, "ERR! wait epfd fflags:%d", events[n].fflags);
            }
#endif
        }
    }

    if (nfds == 0)
    {
        // タイムアウト
        //Logger::print(logid_, "wait epfd TIMEOUT");
        result = 0;
    }
    if (nfds == -1)
    {
        // エラー
        Logger::print(logid_, "ERR! wait epfd err:%d", errno);
        result = -1;
    }

	EV_SET(&kev, sock_, EVFILT_VNODE, EV_DELETE, 0, 0, NULL);
	kevent(epfd_, &kev, 1, NULL, 0, NULL);

    return result;
}

SOCKET ServerSocket::do_accept()
{
    struct sockaddr_in sa_client;
    socklen_t len = sizeof(sa_client);
    SOCKET client = accept(sock_, reinterpret_cast<struct sockaddr *>(&sa_client), &len);
    if (client == INVALID_SOCKET)
    {
        Logger::print(logid_, "ERR! accept client:0x%x err:%d", client, errno);
        return INVALID_SOCKET;
    }

    // ソケットをkqueueの監視対象に加える
	struct kevent kev;
	//EV_SET(&kev, client, EVFILT_VNODE, EV_ADD, NOTE_DELETE | NOTE_READ, 0, NULL);
	EV_SET(&kev, client, EVFILT_READ, EV_ADD, 0, 0, NULL);
	int32_t ret = ::kevent(epfd_, &kev, 1, NULL, 0, NULL);
    if (ret == -1)
    {
        Logger::print(logid_, "ERR! kev_add 2 epfd err:%d", errno);
        return -1;
    }

    connectedSockets_.push_back(client);

    char ip[32];
    memset(ip, 0, sizeof(ip));
    inet_ntop(sa_client.sin_family, &sa_client.sin_addr, ip, sizeof(ip));
    Logger::print(logid_, "accept client:0x%x", client);
    Logger::print(logid_, " -> %s:%d", ip, ntohs(sa_client.sin_port));

    return client;
}

void ServerSocket::do_disconnect(SOCKET sock)
{
	struct kevent kev;
	EV_SET(&kev, sock_, EVFILT_VNODE, EV_DELETE, 0, 0, NULL);
	kevent(epfd_, &kev, 1, NULL, 0, NULL);

    auto itr = connectedSockets_.begin();
    while (itr != connectedSockets_.end())
    {
        if (*itr == sock)
        {
            break;
        }
        ++itr;
    }
    (void)connectedSockets_.erase(itr);
    ::close(sock);
}

void ServerSocket::do_disconnect_all()
{
    auto itr = connectedSockets_.begin();
    while (itr != connectedSockets_.end())
    {
		struct kevent kev;
		EV_SET(&kev, sock_, EVFILT_VNODE, EV_DELETE, 0, 0, NULL);
		kevent(epfd_, &kev, 1, NULL, 0, NULL);
        ::close(*itr);
        itr = connectedSockets_.erase(itr);
    }
}

ClientSocket::ClientSocket(int32_t logid) : Socket(logid)
{
}

ClientSocket::~ClientSocket()
{
}

bool ClientSocket::do_connect(const std::string ipaddr, const uint16_t portNo)
{
    int32_t ret = 0;

    struct sockaddr_in sa_server;
    sa_server.sin_family = AF_INET;
    sa_server.sin_port = htons(portNo);
    inet_pton(sa_server.sin_family, ipaddr.c_str(), &sa_server.sin_addr.s_addr);

    // 接続
    ret = ::connect(sock_, reinterpret_cast<struct sockaddr *>(&sa_server), sizeof(sa_server));
    if (ret != 0)
    {
        Logger::print(logid_, "ERR! connect sock:0x%x err:%d", sock_, errno);
        return false;
    }
    Logger::print(logid_, "connect sock:0x%x", sock_);
    Logger::print(logid_, " -> %s:%d", ipaddr.c_str(), portNo);

    connectedSockets_.push_back(sock_);

    return true;
}

void ClientSocket::do_disconnect()
{
	struct kevent kev;
	EV_SET(&kev, sock_, EVFILT_VNODE, EV_DELETE, 0, 0, NULL);
	kevent(epfd_, &kev, 1, NULL, 0, NULL);

    auto itr = connectedSockets_.begin();
    while (itr != connectedSockets_.end())
    {
        if (*itr == sock_)
        {
            break;
        }
        ++itr;
    }
    (void)connectedSockets_.erase(itr);
}

int32_t ClientSocket::do_send(const char *sndData, const int32_t sndSize)
{
    return Socket::do_send(sock_, sndData, sndSize);
}

int32_t ClientSocket::do_recieve(char **rcvData, int32_t *rcvSize)
{
    return Socket::do_recieve(sock_, rcvData, rcvSize);
}

int32_t ClientSocket::do_recieve_event(const std::function<void(SOCKET, const char *, const int32_t)> &func_recieve)
{
    int32_t result = 0;

    // ソケットをkqueueの監視対象に加える
	struct kevent kev;
	//EV_SET(&kev, sock_, EVFILT_VNODE, EV_ADD, NOTE_DELETE | NOTE_READ, 0, NULL);
	EV_SET(&kev, sock_, EVFILT_READ, EV_ADD, 0, 0, NULL);
	int32_t ret = ::kevent(epfd_, &kev, 1, NULL, 0, NULL);
    if (ret == -1)
    {
        Logger::print(logid_, "ERR! kev_add 3 epfd err:%d", errno);
        return -1;
    }

	static constexpr int32_t MAX_EVENTS = 16;
	struct kevent events[MAX_EVENTS];
	struct timespec timeout;
	timeout.tv_sec = 1;
	timeout.tv_nsec = 0;
	int32_t nfds = ::kevent(epfd_, NULL, 0, events, MAX_EVENTS, &timeout);

    if (nfds > 0)
    {
        if (events[0].flags & EV_EOF)
        {
            // 接続が切れたため、再接続させる
            Logger::print(logid_, "disconnect sock:0x%x", sock_);
            result = 1;
        }
        //else if (events[0].fflags & NOTE_READ)
		else
        {
            // クライアントからデータ受信
            char *data = nullptr;
            int32_t size = 0;
            ret = do_recieve(&data, &size);
            if (ret > 0)
            {
                func_recieve(sock_, data, size);
            }
            else
            {
                // エラー
                result = -1;
            }
            if (data != nullptr)
            {
                delete[] data;
            }
        }
#if 0
        else
        {
            // エラー
            Logger::print(logid_, "ERR! wait epfd fflags:%d", events[0].fflags);
            result = -1;
        }
#endif
    }
    else if (nfds == 0)
    {
        // タイムアウト
        //Logger::print(logid_, "wait epfd TIMEOUT");
        result = 0;
    }
    else
    {
        // エラー
        Logger::print(logid_, "ERR! wait epfd err:%d", errno);
        result = -1;
    }

	EV_SET(&kev, sock_, EVFILT_VNODE, EV_DELETE, 0, 0, NULL);
	kevent(epfd_, &kev, 1, NULL, 0, NULL);

    return result;
}

Server::Reciever::~Reciever()
{
}

Server::Server(int32_t logid) : logid_(logid), serverSock_(logid)
{
}

Server::~Server()
{
    end();
}

void Server::start(Reciever *reciever)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        reciever_ = reciever;
    }

    if (!isRunning_)
    {
        isRunning_ = true;
        std::thread th(&Server::task, this);
        th_.swap(th);
    }
}

void Server::end()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        reciever_ = nullptr;
    }
    if (isRunning_)
    {
        isRunning_ = false;
        th_.join();
    }
}

int32_t Server::sendData(const int32_t id, const char *data, const int32_t size)
{
    return serverSock_.do_send(id, data, size);
}

void Server::task()
{
    Logger::print(logid_, "task sta");

    // ソケットを作成
    if (!serverSock_.do_create())
    {
        return;
    }

    // バインド/リッスン開始
    if (!serverSock_.do_bind_listen(ipaddr_, portNo_))
    {
        return;
    }

    while (isRunning_)
    {
        auto func = [&](SOCKET client, const char *data, const int32_t size)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (reciever_ != nullptr)
            {
                reciever_->recieveData(client, data, size);
            }
        };

        (void)serverSock_.do_recieve_event(func);
    }
    serverSock_.do_disconnect_all();
    serverSock_.do_delete();
    Logger::print(logid_, "task end");
}

Client::Reciever::~Reciever()
{
}

Client::Client(int32_t logid) : logid_(logid), clientSock_(logid)
{
}

Client::~Client()
{
    end();
}

void Client::start(Reciever *reciever)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        reciever_ = reciever;
    }
    if (!isRunning_)
    {
        isRunning_ = true;
        std::thread th(&Client::task, this);
        th_.swap(th);
    }
}

void Client::end()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        reciever_ = nullptr;
    }
    if (isRunning_)
    {
        isRunning_ = false;
        th_.join();
    }
}

int32_t Client::sendData(const char *data, const int32_t size)
{
    return clientSock_.do_send(data, size);
}

void Client::task()
{
    Logger::print(logid_, "task sta");

    while (isRunning_)
    {
        // ソケットを作成
        if (!clientSock_.do_create())
        {
            // リトライ
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // 接続
        if (!clientSock_.do_connect(ipaddr_, portNo_))
        {
            // リトライ
			clientSock_.do_delete();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        while (isRunning_)
        {
            auto func = [&](SOCKET sock, const char *data, const int32_t size)
            {
                std::lock_guard<std::mutex> lock(mtx_);
                if (reciever_ != nullptr)
                {
                    reciever_->recieveData(sock, data, size);
                }
            };

            int32_t result = clientSock_.do_recieve_event(func);
            if (result == 1)
            {
                // 接続が切れたため、再接続させる
                break;
            }
        }
        // 再接続を試みるためソケット破棄
        clientSock_.do_disconnect();
        clientSock_.do_delete();
    }

    Logger::print(logid_, "task end");
}
