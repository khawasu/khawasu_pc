#pragma once
#include <string>
#include <mesh_protocol.h>
#include <logical_device_manager.h>
#include <p2p_unsecured_short_interface.h>
#include <iostream>
#include "phy_device_manager.h"

class KhawasuApp {
public:
    std::string socket_hostname;
    std::uint16_t socket_port;
    std::string fresh_network_name;
    MeshProto::far_addr_t fresh_network_addr;
    std::string fresh_network_psk;

    MeshController* controller;
    LogicalDeviceManager manager;
    PhyDeviceManager* phy_manager = nullptr;

    std::vector<MeshInterface*> interfaces;

    KhawasuApp (std::string freshNetworkName, MeshProto::far_addr_t freshNetworkAddr, std::string freshNetworkPsk);

    bool run () {
        // MeshController::task_check_packets(g_fresh_mesh);
        return true;
    }

    void before_run () {
//        phy_manager = new PhyDeviceManager(&manager);
    }

    void register_fresh_com_device (std::string& path, int boudrate);

    void register_fresh_socket_server (std::string& hostname, uint16_t port);

    void register_fresh_socket_client (std::string& hostname, uint16_t port);

    static inline void mesh_packet_handler (MeshProto::far_addr_t src_phy, const ubyte* data, ushort size,
                                            void* user_data) {
        using namespace LogicalProto;
        using namespace OverlayProto;

        if (size < OVL_PACKET_SIZE(type))
            return;

        auto ovl = (OverlayPacket*) data;
        LogicalPacket* log;
        switch (ovl->type) {
            case OverlayProtoType::RELIABLE: {
                if (size < OVL_PACKET_SIZE(reliable.data))
                    return;
                log = (LogicalPacket*) ovl->reliable.data;
                break;
            }
            case OverlayProtoType::UNRELIABLE: {
                if (size < OVL_PACKET_SIZE(unreliable.data))
                    return;
                log = (LogicalPacket*) ovl->unreliable.data;
                break;
            }
            default: return;
        }

        printf("[khawasu] New packet from %d to %d! %d bytes, type %hhu\n", log->src_addr, log->dst_addr, size,
               log->type);
        fflush(stdout);

        auto app = (KhawasuApp*) user_data;
        app->manager.dispatch_packet(log, size, src_phy);
        if (app->phy_manager)
            app->phy_manager->handle_packet(src_phy, log, size);
    }
};
