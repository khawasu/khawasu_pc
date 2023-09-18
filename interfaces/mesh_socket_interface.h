#pragma once


#include <mesh_base_interface.h>
#include "../socket/socket_base.h"
#include <vector>
#include <exception>

class MeshSocketInterfaceConnectionException : public std::exception
{};

struct ClientRecvBuffer {
    std::vector<ubyte> data;
    uint size = 0;
};

struct Client {
    ClientRecvBuffer buffer;
    SocketHandle_t handle;
};

class MeshSocketInterface: public MeshInterface
{
    bool is_server = false;
public:
    SimpleSecureMeshInterfaceSessionManager<u64> session_manager;
    u64 self_addr;
    u64 addr_counter=0;
    Socket self_sock;
    std::unordered_map<u64, Client> clients;

    MeshSocketInterface(std::string addr, ushort port, bool is_server=false);

    void check_packets() override;

    bool accept_near_packet(MeshPhyAddrPtr phy_addr, const MeshProto::MeshPacket* packet, uint size) override;

    MeshProto::MeshPacket* alloc_near_packet(MeshProto::MeshPacketType type, uint size) override;

    MeshProto::MeshPacket* realloc_near_packet(MeshProto::MeshPacket* packet,
                                               MeshProto::MeshPacketType old_type,
                                               MeshProto::MeshPacketType new_type,
                                               uint new_size) override;

    void free_near_packet(MeshProto::MeshPacket* packet) override;

    void send_packet(MeshPhyAddrPtr phy_addr, const MeshProto::MeshPacket* packet, uint size) override;

    MeshInterfaceProps get_props() override;

    void send_hello(MeshPhyAddrPtr phy_addr) override;

    void write_addr_bytes(MeshPhyAddrPtr phy_addr, void* out_buf) override;

    ~MeshSocketInterface() override;
};
