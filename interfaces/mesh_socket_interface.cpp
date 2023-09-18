#include <iostream>
#include <utility>
#include "mesh_socket_interface.h"
#include <mesh_controller.h>
#include <net_utils.h>

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"


MeshSocketInterface::MeshSocketInterface(std::string addr, ushort port, bool is_server_): is_server(is_server_) {
    if (is_server)
        self_sock.Create(std::move(addr), port);
    else {
        auto serv_addr = self_sock.Connect(std::move(addr), port);
        if(!serv_addr)
            throw MeshSocketInterfaceConnectionException();

        auto serv_addr_u64 = self_sock.AddressU64(serv_addr);

        // todo: remove server addr hardcode
        clients[serv_addr_u64] = Client {ClientRecvBuffer(), self_sock.GetHandle()};
    }
}

void MeshSocketInterface::check_packets() {
    if(is_server) {
        int result = self_sock.GetStatus(self_sock.GetHandle(), STATUS_READ, 0);
        if (result == STATUS_EXCEPT) {
            return;
        }

        if (result == 1) {
            // Accept a client socket
            SocketAddr_t client_addr;
            auto client_sock = self_sock.Accept(client_addr);

            // If new client
            if (client_sock) {
                u64 phy_addr = self_sock.AddressU64(client_addr);
                clients[phy_addr] = {ClientRecvBuffer(), client_sock};
            }
        }
    }

    for(auto& [addr, client] : clients){
        int status = self_sock.GetStatus(client.handle, STATUS_READ, 0);
        if(status != 1) {
            continue;
        }

        if (size_t howMuchInBuffer = self_sock.GetAvailableBytes(client.handle); howMuchInBuffer > 0) {
            // Reading first size field
            if (client.buffer.size == 0 && howMuchInBuffer >= sizeof(uint)) {
                howMuchInBuffer -= sizeof(uint);
                self_sock.Recv(client.handle, (char*)&client.buffer.size, sizeof(uint));
            }

            auto old_size = client.buffer.data.size();
            howMuchInBuffer = std::min(howMuchInBuffer, client.buffer.size - old_size);
            client.buffer.data.resize(client.buffer.data.size() + howMuchInBuffer);

            self_sock.Recv(client.handle, (char*)&client.buffer.data[old_size], howMuchInBuffer);

            if (client.buffer.size == client.buffer.data.size()) {
                controller->on_packet(id, (MeshPhyAddrPtr) &addr, (MeshProto::MeshPacket*)client.buffer.data.data(), client.buffer.size);

                // Cleanup
                client.buffer.data.resize(0);
                client.buffer.size = 0;
            }
        }
    }
}

bool MeshSocketInterface::accept_near_packet(MeshPhyAddrPtr phy_addr, const MeshProto::MeshPacket* packet, uint size) {
    return true;
}

MeshProto::MeshPacket* MeshSocketInterface::alloc_near_packet(MeshProto::MeshPacketType type, uint size) {
    auto ptr = (MeshProto::MeshPacket*) malloc(size);
    ptr->type = type;

    return ptr;
}

MeshProto::MeshPacket* MeshSocketInterface::realloc_near_packet(MeshProto::MeshPacket* packet,
                                                                MeshProto::MeshPacketType old_type,
                                                                MeshProto::MeshPacketType new_type,
                                                                uint new_size) {
    auto ptr = (MeshProto::MeshPacket*) realloc(packet, new_size);
    ptr->type = new_type;

    return ptr;
}

void MeshSocketInterface::free_near_packet(MeshProto::MeshPacket* packet) {
    free(packet);
}

void MeshSocketInterface::send_packet(MeshPhyAddrPtr phy_addr, const MeshProto::MeshPacket* packet, uint size) {
    auto addr = *(u64*)phy_addr;

    if (clients.find(addr) == clients.end()){
        return;
    }

    self_sock.Send(clients[addr].handle, (char*) &size, sizeof(uint));
    self_sock.Send(clients[addr].handle, (char*) packet, size);
}

MeshInterfaceProps MeshSocketInterface::get_props() {
    return {(MeshInterfaceSessionManager*) &session_manager, 256, sizeof(u64), true};
}

void MeshSocketInterface::send_hello(MeshPhyAddrPtr phy_addr) {
    auto packet = (MeshProto::MeshPacket*) alloca(MESH_CALC_SIZE(near_hello_secure));
    net_store(packet->type, MeshProto::MeshPacketType::NEAR_HELLO);
    memcpy(packet->near_hello_secure.network_name, controller->network_name, sizeof(controller->network_name));

    // For specific device
    if (phy_addr != nullptr){
        send_packet(phy_addr, packet, MESH_CALC_SIZE(near_hello_secure));
        return;
    }

    // For all (broadcasting)
    for(auto& [addr, client] : clients){
        send_packet((MeshPhyAddrPtr)&addr, packet, MESH_CALC_SIZE(near_hello_secure));
    }
}

void MeshSocketInterface::write_addr_bytes(MeshPhyAddrPtr phy_addr, void* out_buf) {
    memcpy(out_buf, phy_addr, sizeof(u64));
}

MeshSocketInterface::~MeshSocketInterface() {
    for(auto& [addr, client] : clients){
        self_sock.Close(client.handle);
    }

    self_sock.Close();
}



