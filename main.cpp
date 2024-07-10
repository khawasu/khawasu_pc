#include <iostream>
#include <string>
#include <mesh_controller.h>
#include <logical_device_manager.h>
#include "socket/socket_base.h"
#include "arg_parser.h"
#include "khawasu_app.h"
#include "thirdparties/tomlplusplus/toml.hpp"

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

using namespace LogicalProto;
using namespace OverlayProto;


Socket sock;

int main(int argc, char* argv[]) {
    Config app_config;
    ArgParser argParser;
    argParser.process(argc, argv, &app_config);

    if (!argParser.config->config_path.empty()) {
        std::cout << ":: Load " << argParser.config->config_path << " config" << std::endl;

        if (!perform_config_toml(app_config)) {
            std::cerr << ":: Error occurred while parsing file " << argParser.config->config_path << std::endl;
            return 1;
        }
    }

    if (app_config.debug) {
        std::cout << "Khawasu SDK Devices:" << std::endl;
        for (auto &[dev_enum, dev_type]: KhawasuReflection::getAllDeviceClasses()) {
            std::cout << "\t" << dev_type << " (" << (int) dev_enum << ")" << std::endl;
        }

        std::cout << "Khawasu SDK Actions:" << std::endl;
        for (auto &[act_enum, act_type]: KhawasuReflection::getAllDeviceActions()) {
            std::cout << "\t" << act_type << " (" << (int) act_enum << ")" << std::endl;
        }

        std::cout << "Khawasu SDK Action Statuses:" << std::endl;
        for (auto &[status_enum, status_type]: KhawasuReflection::getAllDeviceActionStatuses()) {
            std::cout << "\t" << status_type << " (" << (int) status_enum << ")" << std::endl;
        }
    }

    std::cout << ":: Khawasu Control Tool (" << GIT_HASH << ") " << std::endl;

    KhawasuApp app(app_config.network_name, app_config.network_addr, app_config.network_psk);

    // Register interfaces
    // Todo: Make COM auto register fresh devices
    for (auto &[is_server, hostname, port]: app_config.socket_interfaces) {
        if (is_server)
            app.register_fresh_socket_server(hostname, port);
        else
            app.register_fresh_socket_client(hostname, port);
    }

    for (auto &[com_addr, com_boudrate]: app_config.com_devices)
        app.register_fresh_com_device(com_addr, com_boudrate);

    // App loop
    while (app.run()) {
        Os::yield_non_starving();
    }

    return 0;
}
