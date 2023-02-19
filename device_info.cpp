#include <iostream>
#include "device_info.h"

LogDeviceInfo::LogDeviceInfo(MeshProto::far_addr_t phy_, LogicalPacket* packet) {
    phy = phy_;
    log = packet->src_addr;
    dev_class = packet->hello_world.device_class;
    name = std::string_view {(char*) packet->hello_world.name, packet->hello_world.name_len};

    auto attrib_ptr = (HelloWorldPacket::HelloWorldDeviceAttrib*) &packet->hello_world.name[packet->hello_world.name_len];
    for (int i = 0; i < packet->hello_world.special_attrib_count; ++i) {
        std::string key{(char*) attrib_ptr->key, attrib_ptr->key_len};
        std::string value{(char*) attrib_ptr->key + attrib_ptr->key_len, attrib_ptr->value_len};

        attribs.emplace(std::move(key), std::move(value));

        attrib_ptr = (HelloWorldPacket::HelloWorldDeviceAttrib*) (attrib_ptr->value + attrib_ptr->key_len +
                                                                  attrib_ptr->value_len);
    }

    auto action_ptr = (HelloWorldPacket::ActionData*) attrib_ptr;
    for (int i = 0; i < packet->hello_world.action_count; ++i) {
        if (action_ptr->name_length == 0)
            continue;

        auto action_name = std::string((const char*) action_ptr->name, action_ptr->name_length);
        auto act_name = new char[action_name.size()+1];
        std::strcpy(act_name, action_name.c_str());

        actions_map.emplace(i, DeviceApiAction(act_name, action_ptr->type));

        action_ptr = (HelloWorldPacket::ActionData*) &action_ptr->name[action_ptr->name_length];
    }
}