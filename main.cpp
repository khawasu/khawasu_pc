#include <iostream>
#include <string>
#include <mesh_controller.h>
#include <logical_device_manager.h>
#include "socket/socket_base.h"
#include "arg_parser.h"
#include "khawasu_app.h"
#include "protocol/Request.h"
#include "thirdparties/tomlplusplus/toml.hpp"
#include <vector>
#include <print>

#include "thirdparties/cppzmq/zmq.hpp"
#include <string>
#include <iostream>

#include "protocol/daemon.h"
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>

#define sleep(n)	Sleep(n)
#endif


#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

using namespace LogicalProto;
using namespace OverlayProto;


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

    //  Prepare our context and socket
    zmq::context_t context (2);
    zmq::socket_t socket (context, zmq::socket_type::rep);
    socket.bind ("tcp://*:7878");

    // App loop
    while (app.run()) {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv (request, zmq::recv_flags::none);

        auto request_span = std::span((char*)request.data(), (char*)request.data() + request.size());

        // Process data
        auto response = DaemonProtocol::ProcessRequest(request_span);

        //  Send reply back to client
        zmq::message_t reply (response.size());
        memcpy (reply.data (), response.data(), response.size());
        socket.send (reply, zmq::send_flags::none);

        // auto requestData = reinterpret_cast<Mess::KhawasuServiceProtocol::Request *>((char *) request.data());
        // auto requestName = std::string(reinterpret_cast<char *   >(requestData->name().values), requestData->name().size);
        // std::cout << "Received request " << requestName << " (" << requestData->name().size << ")" <<  std::endl;
        //
        // auto responseData = Mess::KhawasuServiceProtocol::GetDevicesResponse::Allocate(3);
        // for(int i = 0; i < responseData->devices().size; ++i) {
        //     auto& device = responseData->devices()[i];
        //
        //     auto name = "My awesome device #" + std::to_string(i);
        //     std::strcpy((char*)device.name().values, name.c_str());
        //
        //     auto type = KhawasuReflection::getAllDeviceClasses()[(DeviceClassEnum)(i+1)];
        //     std::strcpy((char*)device.type().values, type.c_str());
        //
        //     auto address_far = "D1 56 0F F1";
        //     std::strcpy((char*)device.address().far().values, address_far);
        //     device.address().log() = 25556;
        // }
        // auto response = Mess::KhawasuServiceProtocol::Response::Allocate(responseData->get_size());
        // std::memcpy(response->data().values, responseData, responseData->get_size());

        Os::yield_non_starving();
    }

    return 0;
}
