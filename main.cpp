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
    ArgParser argParser;
    argParser.process(argc, argv);

    if (!argParser.config_path.empty()) {
        //todo: write INI parser
        std::cout << "Load " << argParser.config_path << " config" << std::endl;
    }

    if (argParser.hostname.empty()){
        argParser.hostname = "127.0.0.1";
    }

    if (argParser.port == 0){
        argParser.port = KHAWASU_DEFAULT_PORT;
    }

    bool isStartServer = (argParser.action_type == "server");

    std::cout << ":: Khawasu Socket Bridge " << std::endl;

    if (isStartServer)
        std::cout << ":: Socket Server :: " << argParser.hostname << ":" << argParser.port << std::endl;
    else
        std::cout << ":: Socket Client :: " << argParser.hostname << ":" << argParser.port << std::endl;

    for(auto& [com_addr, com_boudrate] : argParser.com_devices)
        std::cout << ":: COM Device :: " << com_addr << " with baudrate " << com_boudrate << std::endl;

    std::cout << ":: Fresh network :: \"" << argParser.network_name << "\" by PSK with address = " << argParser.network_addr << std::endl;

    KhawasuApp app(argParser.network_name, argParser.network_addr, argParser.network_psk);

    // Register interfaces
    if (isStartServer)
        app.register_fresh_socket_server(argParser.hostname, argParser.port);
    else
        app.register_fresh_socket_client(argParser.hostname, argParser.port);

    for(auto& [com_addr, com_boudrate] : argParser.com_devices)
        app.register_fresh_com_device(com_addr, com_boudrate);

    // App loop
    while (app.run()) {
        Os::yield_non_starving();
    }

    return 0;
}
