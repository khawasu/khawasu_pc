#pragma once

#include <string>
#include <utility>
#include <vector>
#include <mesh_protocol.h>
#include "config.h"

class ArgParser {
    std::vector<std::string> args;
    int current_index = 0;
    int relative_index = 0;

public:
    std::string config_path{};
    Config* config;

    void process(int argc, char* argv[], Config* app_config){
        config = app_config;

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
            config->network_name = peek();
        }

        if (name == "--addr" || name == "-a") {
            config->network_addr = std::stoi(peek());
        }

        if (name == "--psk") {
            config->network_psk = peek();
        }

        if (name == "--com") {
            config->com_devices.emplace_back(peek(), std::stoi(peek()));
        }

        if (name == "--socket"){
            config->socket_interfaces.emplace_back(peek() == "server", peek(), std::stoi(peek()));
        }

        if (name == "--config" || name == "-c") {
            config_path = peek();
        }

        if (name == "--debug") {
            config->debug = true;
        }
    }

    void processRelativeArgument() {
        auto arg = peek();

        if(relative_index == 0){
            config->action_type = arg;
        }

        if(relative_index == 1){
            config->device_addr = std::stoi(arg);
        }

        if(relative_index == 2){
            config->device_port = std::stoi(arg);
        }

        if(relative_index == 3){
            config->action_name = arg;
        }

        if(relative_index >= 4){
            config->action_values.push_back(arg);

        }

        ++relative_index;
    }
};