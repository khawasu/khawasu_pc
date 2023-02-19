#include <sstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "socket_base.h"

void Socket::Create(std::string addr, ushort port) {
    auto wsadata = new WSADATA;
    WSAStartup(MAKEWORD(2, 0), wsadata);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.S_un.S_addr = inet_addr(addr.c_str());
    address.sin_port = htons(port);

    // Creating socket file descriptor
    SOCKET _handle = socket(AF_INET, SOCK_STREAM, 0);
    if (_handle == INVALID_SOCKET) {
        wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
        exit(1);
    }

    unsigned long argp = 1;
    if (ioctlsocket(_handle, FIONBIO, &argp) == SOCKET_ERROR)
    {
        printf("ioctlsocket() error %d\n", WSAGetLastError());
        exit(1);
    }

    int bind_result = bind(_handle, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    if (bind_result == SOCKET_ERROR) {
        wprintf(L"socket bind failed with error = %d\n", WSAGetLastError());
        exit(1);
    }

    // Listen
    int iResult = listen(_handle, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        Close();
        exit(1);
    }

    printf("Socket server now listen %s:%d\n", addr.c_str(), port);
    fflush(stdout);

    handle = (SocketHandle_t) (new SOCKET(_handle));
};

SocketAddr_t Socket::Connect(std::string addr, ushort port) {
    auto wsadata = new WSADATA;
    WSAStartup(MAKEWORD(2, 0), wsadata);

    auto connection_address = new sockaddr_in;

    memset(connection_address, 0, sizeof(sockaddr_in)); // initialize to zero
    connection_address->sin_family = AF_INET;
    connection_address->sin_port = htons(port);
    connection_address->sin_addr.S_un.S_addr = inet_addr(addr.c_str());

    unsigned long raw_ip_nbo;// = inet_addr(targetIP); // inet_addr is an old method for IPv4
    inet_pton(AF_INET, addr.c_str(), &raw_ip_nbo); // IPv6 method of address acquisition
    if (raw_ip_nbo == INADDR_NONE)
    {
        printf("inet_addr() error \"%s\"\n", addr.c_str());
        return nullptr;
    }

    // Creating socket file descriptor
    SOCKET _handle = socket(AF_INET, SOCK_STREAM, 0);
    if (_handle == INVALID_SOCKET) {
        wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
        return nullptr;
    }

    connection_address->sin_addr.s_addr = raw_ip_nbo;

    int result = connect(_handle, (const sockaddr*)connection_address, sizeof(sockaddr_in));

    if(result == SOCKET_ERROR) {
        printf("Can't connect to socket: %d\n", WSAGetLastError());
        return nullptr;
    }

    handle = (SocketHandle_t) (new SOCKET(_handle));

    return (SocketAddr_t)connection_address;
}


void Socket::Close() {
    closesocket(*((SOCKET*) handle));
}

void Socket::Close(SocketHandle_t _handle) {
    closesocket(*((SOCKET*) _handle));
}

SocketHandle_t Socket::Accept(SocketAddr_t& out_addr) {
    out_addr = (SocketAddr_t) new sockaddr();
    int addr_len = sizeof(sockaddr);

    auto _client_handle = accept(*((SOCKET*) handle), (sockaddr*) out_addr, &addr_len);
    auto client_handle = (SocketHandle_t) new SOCKET(_client_handle);

    if (_client_handle == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        Close(client_handle);
        return nullptr;
    }

    return client_handle;
}

int Socket::Recv(SocketHandle_t _handle, char* buf, size_t buf_len) {
    int read_bytes = recv(*((SOCKET*) _handle), buf, buf_len, 0);
    if (read_bytes == SOCKET_ERROR) {
        wchar_t* s = NULL;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, WSAGetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR) &s, 0, NULL);
        fprintf(stderr, "%S\n", s);
        LocalFree(s);
    } else if (read_bytes != buf_len) {
        //std::cerr << "recv failed with error for client " << data.client_id
        //          << ": read bytes count not equal request bytes" << std::endl;
        //return read_bytes - buf_len;
    }

    return read_bytes;
}

int Socket::Send(SocketHandle_t _handle, char* buf, size_t buf_len) {
    int sent_bytes = send(*((SOCKET*) _handle), buf, (int)buf_len, 0);
    if (sent_bytes == SOCKET_ERROR) {
        wchar_t* s = NULL;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, WSAGetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR) &s, 0, NULL);
        fprintf(stderr, "%S\n", s);
        LocalFree(s);
        return -1;
    }
    if (sent_bytes != buf_len){
        fprintf(stderr, "Error buffer\n");
    }

    return sent_bytes;
}

SocketHandle_t Socket::GetHandle() {
    return handle;
}

std::string Socket::AddressString(SocketAddr_t addr) {
    auto address = (sockaddr*) addr;
    std::stringstream ss;
    ss << (int) (address->sa_data[2] & 0xff)
       << "." << (int) (address->sa_data[3] & 0xff)
       << "." << (int) (address->sa_data[4] & 0xff)
       << "." << (int) (address->sa_data[5] & 0xff)
       << ":" << (address->sa_data[0] & 0xff) * 256 + (address->sa_data[1] & 0xff);

    return ss.str();
}

u64 Socket::AddressU64(SocketAddr_t addr) {
    auto address = (sockaddr_in*) addr;

    u64 result = 0;
    std::memcpy(&result, &address->sin_addr.S_un.S_addr, 6);
    std::memcpy(&((ubyte*)&result)[6], &address->sin_port, 2);
    return result;
}

// status: 0x1 for read, 0x2 for write, 0x4 for exception
int Socket::GetStatus(SocketHandle_t sock_handle, int status, int microseconds)
{
    SOCKET a_socket = *(SOCKET*)sock_handle;

    // zero seconds, zero milliseconds. max time select call allowd to block
    static timeval instantSpeedPlease = { 0, microseconds };
    fd_set a = { 1, {a_socket} };
    fd_set* read = ((status & 0x1) != 0) ? &a : NULL;
    fd_set* write = ((status & 0x2) != 0) ? &a : NULL;
    fd_set* except = ((status & 0x4) != 0) ? &a : NULL;
    /*
    select returns the number of ready socket handles in the fd_set structure, zero if the time limit expired, or SOCKET_ERROR if an error occurred. WSAGetLastError can be used to retrieve a specific error code.
    */
    int result = select(0, read, write, except, &instantSpeedPlease);
    if (result == SOCKET_ERROR)
    {
        result = WSAGetLastError();
    }
    if (result < 0 || result > 3)
    {
        printf("select(read) error %d\n", result);
        return 0x4;
    }
    return result;
}

unsigned long Socket::GetAvailableBytes(SocketHandle_t handle) {
    unsigned long howMuchInBuffer = 0;
    ioctlsocket(*(SOCKET*)handle, FIONREAD,
                &howMuchInBuffer);

    return howMuchInBuffer;
}