#pragma once


#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include "socket_base.h"
#include "fcntl.h"
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void Socket::Create(std::string addr, ushort port) {
    std::cout << "Hewwo from GNU/Linux" << std::endl;

    sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = htonl(INADDR_ANY); // Todo: Add support to string addr

    int* _handle = new int(socket(AF_INET, SOCK_STREAM, 0));
    handle = _handle;

    int optval = 1;
    setsockopt(*_handle, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (bind(GetHandleInt(), (struct sockaddr*) &_addr, sizeof(_addr)) < 0) {
        perror("bind");
        exit(2);
    }

    listen(GetHandleInt(), 10);
};

void Socket::Close() {
    close(GetHandleInt());
}

void Socket::Close(SocketHandle_t _handle) {
    close(GetHandleInt(_handle));
}

SocketHandle_t Socket::Accept(SocketAddr_t& outAddr) {
    outAddr = new sockaddr();
    socklen_t addrSize = sizeof(sockaddr);
    int* _handle = new int(accept(GetHandleInt(), (sockaddr*) outAddr, &addrSize));

    SocketHandle_t clientHandle = _handle;
    if (*_handle < 0) {
        perror("accept");
        exit(3);
    }

    return clientHandle;
}

int Socket::Recv(SocketHandle_t _handle, char* buf, int buf_len) {
    int read_bytes = recv(GetHandleInt(_handle), (void*) buf, buf_len, 0);
    if (read_bytes < 0) {
        std::cerr << "recv failed with error: " << std::endl;
    } else if (read_bytes != buf_len) {
        //std::cerr << "recv failed with error for client " << data.client_id
        //          << ": read bytes count not equal request bytes" << std::endl;
        //return read_bytes - buf_len;
    }

    return read_bytes;
}

int Socket::Send(SocketHandle_t _handle, char* buf, int buf_len) {
    int sent_bytes = send(GetHandleInt(_handle), (void*) buf, buf_len, 0);
    if (sent_bytes < 0) {
        printf("Send failed with error\n");
        return -1;
    }

    return sent_bytes;
}

SocketHandle_t Socket::GetHandle() {
    return handle;
}

int Socket::GetHandleInt() {
    return *((int*) handle);
}

int Socket::GetHandleInt(SocketAddr_t _handle) {
    return *((int*) _handle);
}

std::string Socket::AddressString(SocketAddr_t addr) {
    auto address = (sockaddr_in*) addr;
    std::stringstream ss;
    ss << inet_ntoa(address->sin_addr);

    return ss.str();
}

u64 Socket::AddressU64(SocketAddr_t addr) {
    auto address = (sockaddr_in*) addr;

    u64 result;
    std::memcpy(&result, address->s_addr, 6);
    return result;
}

