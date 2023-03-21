#pragma once

#include <new>
#include <random>
#include <unordered_map>
#include <climits>
#include <mesh_protocol.h>
#include "device_info.h"
#include <net_utils.h>

#define DEV_CONTROLLER_NAME "Controller"
#define DEV_MAN_RANGE_MIN 6000
#define DEV_MAN_RANGE_MAX 7000
#define DEV_CONTROLLER_PORT 5999
#define DEV_MAN_RESPONSE_TIMEOUT 5'000'000

class DeviceController : public LogicalDevice {
public:
    OVERRIDE_DEV_CLASS(DeviceClassEnum::CONTROLLER);

    explicit DeviceController(LogicalDeviceManager* manager, const char* name_, ushort port_)
            : LogicalDevice(manager, name_, port_) {}

};

struct IncomingPacket {
    MeshProto::far_addr_t src_phy;
    ushort src_port;
    ushort size;
    LogicalPacket* log;
};

typedef std::function<void(LogDeviceInfo)> NewDeviceCallback;

class PhyDeviceManager {
public:
    std::unordered_map<MeshProto::far_addr_t, PhyDeviceInfo> known_devices;
    LogicalDeviceManager* device_manager;
    DeviceController controller;
    std::unordered_map<ubyte, ActionExecuteStatus> action_exec_responses;
    std::unordered_map<ubyte, IncomingPacket> action_fetch_responses;
    ubyte action_exec_responses_counter = 0;
    ubyte action_fetch_responses_counter = 0;
    NewDeviceCallback new_device_callback;


    static MeshProto::far_addr_t get_self_addr();

    ushort gen_uniq_port();

    void send_hello();

    void handle_packet(MeshProto::far_addr_t src_phy, LogicalPacket* log, ushort size);

    PhyDeviceInfo* lookup(MeshProto::far_addr_t addr);

    LogDeviceInfo* lookup_log(MeshProto::far_addr_t addr, ushort port);

    std::pair<const int, DeviceApiAction>* lookup_action(MeshProto::far_addr_t addr, ushort port, const char* name);

    ActionExecuteStatus
    execute_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer,
                   bool require_status);

    ActionExecuteStatus
    execute_action(MeshProto::far_addr_t addr, ushort port, const char* action_name, std::vector<ubyte> buffer,
                   bool require_status);

    std::vector<ubyte>*
    fetch_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer);

    std::vector<ubyte>*
    fetch_action(MeshProto::far_addr_t addr, ushort port, const char* action_name, std::vector<ubyte> buffer);

    explicit PhyDeviceManager(LogicalDeviceManager* device_manager_);
};
