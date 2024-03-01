#pragma once

#include <string>
#include <mesh_protocol.h>
#include "thirdparties/tomlplusplus/toml.hpp"

struct ComInterface {
public:
    std::string path;
    int boudrate;

    ComInterface(std::string path, int boudrate) : path(std::move(path)), boudrate(boudrate) {}

};

struct SocketInterface {
public:
    bool is_server;
    std::string hostname;
    short port;

    SocketInterface(bool isServer, std::string hostname, short port) : is_server(isServer),
                                                                       hostname(std::move(hostname)),
                                                                       port(port) {}
};

struct Config {
    std::string network_name;
    MeshProto::far_addr_t network_addr = 0;
    std::string network_psk;
    std::vector<ComInterface> com_devices;
    std::vector<SocketInterface> socket_interfaces;
    std::string config_path;
    std::string action_type;
    MeshProto::far_addr_t device_addr = 0;
    int device_port = 0;
    std::string action_name;
    std::vector<std::string> action_values;

    bool debug = false;
};

static inline bool perform_config_toml(Config& app_config){
    toml::table config;
    try {
        config = toml::parse_file(app_config.config_path);
    } catch (const toml::parse_error& err) {
        std::cerr << ":: Parsing failed:\n" << err << std::endl;
        return false;
    }

    app_config.network_addr = config["fresh"]["addr"].value_or(0);
    app_config.network_name = config["fresh"]["net"].value_or("");
    app_config.network_psk = config["fresh"]["psk"].value_or("");

    auto interfaces = config["fresh"]["interfaces"].as<toml::table>();
    for(auto& [_interface_type, _interfaces] : *interfaces){
        auto interface_type = std::string(_interface_type.str());
        std::transform(interface_type.begin(), interface_type.end(), interface_type.begin(), [](unsigned char c){ return std::tolower(c); });

        for(auto& [key, value] : *_interfaces.as_table()) {
            auto valueTable = *value.as_table();

            if (interface_type == "socket") {
                app_config.socket_interfaces.emplace_back("server" == valueTable["type"],
                                                         valueTable["host"].value_or(""),
                                                         valueTable["port"].value_or(0));
            } else if (interface_type == "com") {
                app_config.com_devices.emplace_back(valueTable["path"].value_or(""), valueTable["baud"].value_or(0));
            }
        }
    }

    return true;
}