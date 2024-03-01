#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sstream>
#include "socket_base.h"
#include "fcntl.h"
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>


void Socket::Create(std::string addr, ushort port) {
    std::cout << "Hewwo from GNU/Linux" << std::endl;

    sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = htonl(INADDR_ANY); // Todo: Add support to string addr

    int* _handle = new int(socket(AF_INET, SOCK_STREAM, 0));
    handle = (SocketHandle_t) (_handle);

    int optval = 1;
    setsockopt(*_handle, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (bind(*_handle, (struct sockaddr*) &_addr, sizeof(_addr)) < 0) {
        perror("bind");
        exit(2);
    }

    int listenResult = 0;
    if((listenResult = listen(*_handle, 10)) < 0) {
        printf("Listen error %d\n", listenResult); fflush(stdout);
        exit(2);
    }

    // printf("Listen result %d\n", listenResult); fflush(stdout);
};

SocketAddr_t Socket::Connect(std::string addr, ushort port) {
    auto server_addr = new sockaddr_in();

    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        std::cout << "Error establishing socket..." << std::endl;
        return nullptr;
    }

    std::cout << "Socket client has been created..." << std::endl;

    std::memset(server_addr, 0, sizeof(sockaddr_in)); // initialize to zero
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);

    inet_aton(addr.c_str(), &server_addr->sin_addr);

    if (connect(client, (sockaddr *) server_addr, sizeof(server_addr)) == 0) {
        std::cout << "Connection to the server " << addr << ":" << port << std::endl;
    } else 
        std::cout << "Failed connection to the server " << addr << ":" <<  port << std::endl;

    handle = (SocketHandle_t) new int(client);

    return (SocketAddr_t) server_addr;
}

// status: 0x1 for read, 0x2 for write, 0x4 for exception
int Socket::GetStatus(SocketHandle_t sock_handle, int status, int microseconds)
{
    int a_socket = *(int*)sock_handle;

    // zero seconds, zero milliseconds. max time select call allowd to block
    static timeval instantSpeedPlease = { 0, microseconds };
    // fd_set a = { 1, a_socket };
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(a_socket, &rfds);

    fd_set* read = ((status & 0x1) != 0) ? &rfds : NULL;
    fd_set* write = ((status & 0x2) != 0) ? &rfds : NULL;
    fd_set* except = ((status & 0x4) != 0) ? &rfds : NULL;
    /*
    select returns the number of ready socket handles in the fd_set structure, zero if the time limit expired, or SOCKET_ERROR if an error occurred. WSAGetLastError can be used to retrieve a specific error code.
    */
    int result = select(a_socket + 1, read, write, except, &instantSpeedPlease);
    if (result == -1)
    {
        result = errno;
    }
    if (result < 0 || result > 3)
    {
        printf("select(read) error %d\n", result);
        return 0x4;
    }
    return result;
}

void Socket::Close() {
    close(*(int*)GetHandle());
}

void Socket::Close(SocketHandle_t _handle) {
    close(*(int*)_handle);
}

SocketHandle_t Socket::Accept(SocketAddr_t& outAddr) {
    outAddr = (SocketAddr_t)  new sockaddr();
    socklen_t addrSize = sizeof(sockaddr);
    int* _handle = new int(accept(*(int*)GetHandle(), (sockaddr*) outAddr, &addrSize));

    SocketHandle_t clientHandle = (SocketHandle_t)  _handle;
    if (*_handle < 0) {
        perror("accept");
        exit(3);
    }

    return clientHandle;
}

int Socket::Recv(SocketHandle_t _handle, char* buf, size_t buf_len) {
    int read_bytes = recv(*(int*)_handle, (void*) buf, buf_len, 0);
    if (read_bytes < 0) {
        std::cerr << "recv failed with error: " << std::endl;
    } else if (read_bytes != buf_len) {
        //std::cerr << "recv failed with error for client " << data.client_id
        //          << ": read bytes count not equal request bytes" << std::endl;
        //return read_bytes - buf_len;
    }

    return read_bytes;
}

int Socket::Send(SocketHandle_t _handle, char* buf, size_t buf_len) {
    int sent_bytes = send(*(int*)_handle, (void*) buf, buf_len, 0);
    if (sent_bytes < 0) {
        printf("Send failed with error\n");
        return -1;
    }

    return sent_bytes;
}

SocketHandle_t Socket::GetHandle() {
    return handle;
}

// int Socket::GetHandleInt() {
//     return *((int*) handle);
// }

// int Socket::GetHandleInt(SocketAddr_t _handle) {
//     return *((int*) _handle);
// }

std::string Socket::AddressString(SocketAddr_t addr) {
    auto address = (sockaddr_in*) addr;
    std::stringstream ss;
    ss << inet_ntoa(address->sin_addr);

    return ss.str();
}

u64 Socket::AddressU64(SocketAddr_t addr) {
    auto address = (sockaddr_in*) addr;

    u64 result;
    std::memcpy(&result, &address->sin_addr, 6);
    return result;
}

unsigned long Socket::GetAvailableBytes(SocketHandle_t handle) {
    unsigned long howMuchInBuffer = 0;
    ioctl(*(int*)handle, FIONREAD, &howMuchInBuffer);

    return howMuchInBuffer;
}
