#pragma once

#include <string_view>
#include "types.h"


typedef struct SocketAddr_s* SocketAddr_t;
typedef struct SocketHandle_s* SocketHandle_t;

const int STATUS_READ = 0x1, STATUS_WRITE = 0x2, STATUS_EXCEPT = 0x4; // used by getStatus

class Socket {
public:
    void Create(std::string addr, ushort port);

    SocketAddr_t Connect(std::string addr, ushort port);

    void Close();

    void Close(SocketHandle_t handle);

    SocketHandle_t Accept(SocketAddr_t& outAddr);

    unsigned long GetAvailableBytes(SocketHandle_t handle);

    int Recv(SocketHandle_t handle, char* buf, size_t buf_len);

    int Send(SocketHandle_t handle, char* buf, size_t buf_len);

    SocketHandle_t GetHandle();

    std::string AddressString(SocketAddr_t addr);

    u64 AddressU64(SocketAddr_t addr);

    int GetStatus(SocketHandle_t sock_handle, int status, int microseconds);

protected:
    SocketHandle_t handle;
};