#pragma once

#include <logical_device.h>

using namespace LogicalProto;


struct LogDeviceInfo {
public:
    MeshProto::far_addr_t phy;
    ushort log;
    DeviceClassEnum dev_class;
    std::string name;
    std::unordered_map<std::string, std::string> attribs;
    std::unordered_map<int, DeviceApiAction> actions_map;

    explicit LogDeviceInfo(MeshProto::far_addr_t phy, LogicalPacket* packet);
};

struct PhyDeviceInfo {
    std::unordered_map<ushort, LogDeviceInfo> log_devices;

};