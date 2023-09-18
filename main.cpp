#include <iostream>
#include <mesh_controller.h>
#include <platform/p2p/win32_p2p.h>
#include <p2p_unsecured_short_interface.h>
#include <logical_device_manager.h>
#include <to_fix.h>
#include <preserved_property.h>
#include "socket/socket_base.h"
#include "interfaces/mesh_socket_interface.h"
#include "arg_parser.h"
#include "khawasu_app.h"
#include "thirdparties/tomlplusplus/toml.hpp"

#define KHAWASU_DEFAULT_PORT 48655

using namespace LogicalProto;
using namespace OverlayProto;

inline void mesh_packet_handler(MeshProto::far_addr_t src_phy, const ubyte* data, ushort size, void* user_data) {
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

    printf("[khawasu] New packet from %d to %d! %d bytes, type %hhu\n", log->src_addr, log->dst_addr, size, log->type);
    fflush(stdout);

    ((LogicalDeviceManager*)user_data)->dispatch_packet(log, size, src_phy);
}

Socket sock;

int main(int argc, char* argv[]) {
    Config app_config;
    ArgParser argParser;
    argParser.process(argc, argv, &app_config);

    if (!argParser.config->config_path.empty()) {
        std::cout << ":: Load " << argParser.config->config_path << " config" << std::endl;

        if(!perform_config_toml(app_config)) {
            std::cerr << ":: Error occurred while parsing file " << argParser.config->config_path << std::endl;
            return 1;
        }
    }

    if(app_config.debug) {
        std::cout << "Khawasu SDK Devices:" << std::endl;
        for (auto& [dev_enum, dev_type]: KhawasuReflection::getAllDeviceClasses()) {
            std::cout << "\t" << dev_type << " (" << (int) dev_enum << ")" << std::endl;
        }

        std::cout << "Khawasu SDK Actions:" << std::endl;
        for (auto& [act_enum, act_type]: KhawasuReflection::getAllDeviceActions()) {
            std::cout << "\t" << act_type << " (" << (int) act_enum << ")" << std::endl;
        }

        std::cout << "Khawasu SDK Action Statuses:" << std::endl;
        for (auto& [status_enum, status_type]: KhawasuReflection::getAllDeviceActionStatuses()) {
            std::cout << "\t" << status_type << " (" << (int) status_enum << ")" << std::endl;
        }
    }

    std::cout << ":: Khawasu Control Tool " << std::endl;

    KhawasuApp app(app_config.network_name, app_config.network_addr, app_config.network_psk);

    // Register interfaces
    // Todo: Make COM auto register fresh devices
    for(auto& [is_server, hostname, port] : app_config.socket_interfaces){
        if (is_server)
            app.register_fresh_socket_server(hostname, port);
        else
            app.register_fresh_socket_client(hostname, port);
    }

    for(auto& [com_addr, com_boudrate] : app_config.com_devices)
        app.register_fresh_com_device(com_addr, com_boudrate);

    // App loop
    while (app.run()) {
        Os::sleep_ticks(100);
    }

    return 0;
}
