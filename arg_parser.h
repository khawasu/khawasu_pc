#pragma once

#include <string>
#include <utility>
#include <vector>
#include <mesh_protocol.h>

struct ComDevice {
public:
    std::string path;
    int boudrate;

    ComDevice(std::string path, int boudrate) : path(std::move(path)), boudrate(boudrate) {}

};

struct SocketInterface {
public:
    bool is_server;
    std::string hostname;
    short port;

    SocketInterface(bool isServer, std::string  hostname, short port) : is_server(isServer),
                                                                        hostname(std::move(hostname)),
                                                                        port(port) {}
};

class ArgParser {
    std::vector<std::string> args;
    int current_index = 0;
    int relative_index = 0;

public:
    std::string network_name;
    MeshProto::far_addr_t network_addr = 0;
    std::string network_psk;
    std::vector<ComDevice> com_devices;
    std::vector<SocketInterface> socket_interfaces;
    std::string config_path;
    std::string action_type;
    MeshProto::far_addr_t device_addr = 0;
    int device_port = 0;
    std::string action_name;
    std::vector<std::string> action_values;

    bool debug = false;

    void process(int argc, char* argv[]){
        for (int i=1; i<argc; ++i){
            args.emplace_back(argv[i]);
        }

        current_index = 0;
        relative_index = 0;
        while(current_index < args.size()){
            auto arg = seek();
            if (arg.find('-') == 0)
                processNamedArgument();
            else
                processRelativeArgument();
        }
    }

private:
    std::string seek(int offset = 0){
        if(current_index + offset >= args.size())
            return "";
        return args[current_index + offset];
    }

    std::string peek(){
        auto arg = seek();
        ++current_index;
        return arg;
    }

    void processNamedArgument(){
        auto name = peek();

        if (name == "--net" || name == "-n") {
            network_name = peek();
        }

        if (name == "--addr" || name == "-a") {
            network_addr = std::stoi(peek());
        }

        if (name == "--psk") {
            network_psk = peek();
        }

        if (name == "--com") {
            com_devices.emplace_back(peek(), std::stoi(peek()));
        }

        if (name == "--socket"){
            socket_interfaces.emplace_back(peek() == "server", peek(), std::stoi(peek()));
        }

        if (name == "--config" || name == "-c") {
            config_path = peek();
        }

        if (name == "--debug") {
            debug = true;
        }
    }

    void processRelativeArgument() {
        auto arg = peek();

        if(relative_index == 0){
            action_type = arg;
        }

        if(relative_index == 1){
            device_addr = std::stoi(arg);
        }

        if(relative_index == 2){
            device_port = std::stoi(arg);
        }

        if(relative_index == 3){
            action_name = arg;
        }

        if(relative_index >= 4){
            action_values.push_back(arg);

        }

        ++relative_index;
    }
};