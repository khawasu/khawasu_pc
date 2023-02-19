#include "phy_device_manager.h"

#include <utility>
#include <iostream>

MeshProto::far_addr_t PhyDeviceManager::get_self_addr() {
    return g_fresh_mesh->self_addr;
}

ushort PhyDeviceManager::gen_uniq_port() {
    for (ushort port = DEV_MAN_RANGE_MIN; port < DEV_MAN_RANGE_MAX; ++port) {
        if (lookup_log(get_self_addr(), port) == nullptr)
            return port;
    }

    return 0;
}

void PhyDeviceManager::send_hello() {
    //controller.send_hello_world(LogicalPacketType::HELLO_WORLD, MeshProto::BROADCAST_FAR_ADDR, LogicalProto::BROADCAST_PORT);
}

void PhyDeviceManager::handle_packet(MeshProto::far_addr_t src_phy, LogicalPacket* log, ushort size) {
    if (log->type == LogicalPacketType::HELLO_WORLD || log->type == LogicalPacketType::HELLO_WORLD_RESPONSE) {
        if (known_devices.find(src_phy) == known_devices.end())
            known_devices.emplace(src_phy, PhyDeviceInfo());

        auto device_info = LogDeviceInfo(src_phy, log);
        if(!known_devices.at(src_phy).log_devices.contains(log->src_addr))
            new_device_callback(device_info);

        known_devices.at(src_phy).log_devices.emplace(log->src_addr, device_info);

        printf("[khawasu] New log device (%d, %d) %d\n", src_phy, log->src_addr, known_devices.at(src_phy).log_devices.at(log->src_addr).dev_class);
    }

    if (log->type == LogicalPacketType::ACTION_EXECUTE_RESULT) {
        action_exec_responses.emplace(log->action_execute_result.request_id, log->action_execute_result.status);
    }

    if (log->type == LogicalPacketType::ACTION_RESPONSE) {
        action_fetch_responses.emplace(log->action_response.request_id,
                                       IncomingPacket{src_phy, log->src_addr, size, log});
    }
}

PhyDeviceInfo* PhyDeviceManager::lookup(MeshProto::far_addr_t addr) {
    if (known_devices.find(addr) == known_devices.end()) {
        printf("[khawasu] Not found phy (%d)\n", addr);
        return nullptr;
    }

    return &known_devices.at(addr);
}

LogDeviceInfo* PhyDeviceManager::lookup_log(MeshProto::far_addr_t addr, ushort port) {
    auto phy = lookup(addr);
    if (phy == nullptr || phy->log_devices.find(port) == phy->log_devices.end()) {
        printf("[khawasu] Not found log (%d, %d)\n", addr, port);
        return nullptr;
    }

    return &phy->log_devices.at(port);
}

std::pair<const int, DeviceApiAction>*
PhyDeviceManager::lookup_action(MeshProto::far_addr_t addr, ushort port, const char* name) {
    auto dev = lookup_log(addr, port);
    if (dev == nullptr)
        return nullptr;

    for (auto& pair: dev->actions_map) {
        if (strcmp(pair.second.name, name) == 0)
            return &pair;
    }

    return nullptr;
}

ActionExecuteStatus
PhyDeviceManager::execute_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer,
                                 bool require_status) {
    ubyte req_id = action_exec_responses_counter++;
    auto log = device_manager->alloc_logical_packet_ptr(LogicalAddress(addr, port), controller.self_port, buffer.size(),
                                                        OverlayProtoType::UNRELIABLE,
                                                        LogicalPacketType::ACTION_EXECUTE);
    net_store(log.ptr()->action_execute.request_id, req_id);
    net_store(log.ptr()->action_execute.action_id, action_id);
    net_memcpy((ubyte*) log.ptr()->action_execute.payload, buffer.data(), buffer.size());
    device_manager->finish_ptr(log);

    if(!require_status)
        return ActionExecuteStatus::UNKNOWN;


    auto start_time = Os::get_microseconds();
    auto status = ActionExecuteStatus::TIMEOUT;
    while (status == ActionExecuteStatus::TIMEOUT &&
           Os::get_microseconds() - start_time < DEV_MAN_RESPONSE_TIMEOUT) {

        auto it = action_exec_responses.find(req_id);
        if (it != action_exec_responses.end()) {
            status = action_exec_responses.at(req_id);
            action_exec_responses.erase(it);
        }

        Os::yield_non_starving();
    }

    return status;
}

ActionExecuteStatus PhyDeviceManager::execute_action(MeshProto::far_addr_t addr, ushort port, const char* action_name,
                                                     std::vector<ubyte> buffer, bool require_status) {
    auto action = lookup_action(addr, port, action_name);
    if (action == nullptr)
        return LogicalProto::ActionExecuteStatus::ACTION_NOT_FOUND;

    return execute_action(addr, port, action->first, std::move(buffer), require_status);
}

std::vector<ubyte>*
PhyDeviceManager::fetch_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer) {
    ubyte req_id = action_fetch_responses_counter++;
    auto log = device_manager->alloc_logical_packet_ptr(LogicalAddress(addr, port), controller.self_port, buffer.size(),
                                                        OverlayProtoType::UNRELIABLE,
                                                        LogicalPacketType::ACTION_FETCH);
    net_store(log.ptr()->action_fetch.request_id, req_id);
    net_store(log.ptr()->action_fetch.action_id, action_id);
    net_memcpy((ubyte*) log.ptr()->action_fetch.payload, buffer.data(), buffer.size());
    device_manager->finish_ptr(log);

    auto start_time = Os::get_microseconds();
    std::vector<ubyte>* result = nullptr;
    while (result == nullptr &&
           Os::get_microseconds() - start_time < DEV_MAN_RESPONSE_TIMEOUT) {

        auto it = action_fetch_responses.find(req_id);
        if (it != action_fetch_responses.end()) {
            auto packet = action_fetch_responses.at(req_id);
            result = new std::vector<ubyte>(packet.log->action_response.payload,
                                            &packet.log->action_response.payload[packet.size -
                                                                                 LOG_PACKET_SIZE(action_response)]);
            action_fetch_responses.erase(it);
        }

        Os::yield_non_starving();
    }

    return result;
}

std::vector<ubyte>* PhyDeviceManager::fetch_action(MeshProto::far_addr_t addr, ushort port, const char* action_name,
                                                   std::vector<ubyte> buffer) {
    auto action = lookup_action(addr, port, action_name);
    if (action == nullptr)
        return nullptr;

    return fetch_action(addr, port, action->first, std::move(buffer));
}

