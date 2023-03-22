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
    ArgParser argParser;
    argParser.process(argc, argv);

    if (!argParser.config_path.empty()) {
        std::cout << ":: Load " << argParser.config_path << " config" << std::endl;

        toml::table config;
        try {
            config = toml::parse_file(argParser.config_path);
        } catch (const toml::parse_error& err) {
            std::cerr << ":: Parsing failed:\n" << err << std::endl;
            return 1;
        }

        if (config["fresh"]["addr"].is_integer()) argParser.network_addr = config["fresh"]["addr"].value_or(0);
        if (config["fresh"]["net"].is_string()) argParser.network_name = config["fresh"]["net"].value_or("");
        if (config["fresh"]["psk"].is_string()) argParser.network_psk = config["fresh"]["psk"].value_or("");

        auto interfaces = config["fresh"]["interfaces"].as<toml::table>();
        for(auto& [_interface_type, _interfaces] : *interfaces){
            auto interface_type = std::string(_interface_type.str());
            std::transform(interface_type.begin(), interface_type.end(), interface_type.begin(), [](unsigned char c){ return std::tolower(c); });

            for(auto& [key, value] : *_interfaces.as_table()) {
                auto valueTable = *value.as_table();

                if (interface_type == "socket") {
                    argParser.socket_interfaces.emplace_back("server" == valueTable["type"],
                                                             valueTable["host"].value_or(""),
                                                             valueTable["port"].value_or(0));
                } else if (interface_type == "com") {
                    argParser.com_devices.emplace_back(valueTable["path"].value_or(""), valueTable["baud"].value_or(0));
                }
            }
        }
    }

    std::cout << ":: Khawasu Control Tool " << std::endl;

    KhawasuApp app(argParser.network_name, argParser.network_addr, argParser.network_psk);

    // Register interfaces
    for(auto& [is_server, hostname, port] : argParser.socket_interfaces){
        if (is_server)
            app.register_fresh_socket_server(hostname, port);
        else
            app.register_fresh_socket_client(hostname, port);
    }

    for(auto& [com_addr, com_boudrate] : argParser.com_devices)
        app.register_fresh_com_device(com_addr, com_boudrate);

    // App loop
    while (app.run()) {
        Os::yield_non_starving();
    }

    return 0;
}
