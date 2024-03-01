#pragma once

#include <logical_device.h>

using namespace LogicalProto;

class PhyDeviceManager;

enum class ActionResponseType {
    UNKNOWN = 0, FETCH, EXECUTE, SUBSCRIPTION_CALLBACK
};

struct ActionResponse {
    bool is_waiting;
    unsigned int req_id;
    ActionResponseType type;
    MeshProto::far_addr_t src_phy;
    ushort src_port;
    ActionExecuteStatus status;
    std::vector<ubyte> payload;

    u64 time_sent = 0;
    u64 time_received = 0;

    ActionResponse(unsigned int req_id, ActionResponseType type, MeshProto::far_addr_t src_phy, ushort src_port, u64 time_sent)
            : req_id(req_id),
              type(type),
              src_phy(src_phy),
              src_port(src_port),
              time_sent(time_sent),
              is_waiting(true) {}
};

struct LogDeviceInfo {
public:
    PhyDeviceManager* manager;
    MeshProto::far_addr_t phy;
    ushort log;
    DeviceClassEnum dev_class;
    std::string name;
    std::unordered_map<std::string, std::string> attribs;
    std::unordered_map<int, DeviceApiAction> actions_map;

    LogDeviceInfo(PhyDeviceManager* manager, MeshProto::far_addr_t phy, LogicalPacket* packet);

    ActionExecuteStatus execute_action(ushort action_id, std::vector<ubyte> buffer, bool require_status = false);

    ActionExecuteStatus execute_action(const char* action_name, std::vector<ubyte> buffer, bool require_status = false);

    ActionResponse* fetch_action(ushort action_id, std::vector<ubyte> buffer);

    ActionResponse* fetch_action(const char* action_name, std::vector<ubyte> buffer);
};

struct PhyDeviceInfo {
    std::unordered_map<ushort, LogDeviceInfo> log_devices;

};