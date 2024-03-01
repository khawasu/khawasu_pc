#include <logical_device_manager.h>
#include "phy_device_manager.h"

#include <utility>
#include <iostream>
#include <cassert>

using namespace OverlayProto;

MeshProto::far_addr_t PhyDeviceManager::get_self_addr() {
    return g_fresh_mesh->self_addr;
}

ushort PhyDeviceManager::gen_uniq_port() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 6);

    return distrib(gen);
}

void PhyDeviceManager::send_hello() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    controller.send_hello_world(LogicalPacketType::HELLO_WORLD, MeshProto::BROADCAST_FAR_ADDR,
                                LogicalProto::BROADCAST_PORT);
}

void PhyDeviceManager::handle_packet(MeshProto::far_addr_t src_phy, LogicalPacket* log, ushort size) {
    if (log->type == LogicalPacketType::HELLO_WORLD || log->type == LogicalPacketType::HELLO_WORLD_RESPONSE) {
        if (known_devices.find(src_phy) == known_devices.end())
            known_devices.emplace(src_phy, PhyDeviceInfo());

        bool is_new_device = false;
        auto device_info = LogDeviceInfo(this, src_phy, log);
        if (!known_devices.at(src_phy).log_devices.contains(log->src_addr)) {
            std::cout << device_info.phy << ":" << device_info.log << " "
                    << KhawasuReflection::getAllDeviceClasses()[device_info.dev_class] << std::endl;

            is_new_device = true;
        }

        known_devices.at(src_phy).log_devices.emplace(log->src_addr, device_info);

        printf("[khawasu] New log device (%d, %d) %d\n", src_phy, log->src_addr,
               known_devices.at(src_phy).log_devices.at(log->src_addr).dev_class);

        if (is_new_device && new_device_callback)
            new_device_callback(device_info);
    }

    if (log->type == LogicalPacketType::ACTION_EXECUTE_RESULT) {
        if (auto data = action_responses.find(log->action_execute_result.request_id); data != action_responses.end()) {
            assert(data->second.is_waiting);

            data->second.status = log->action_execute_result.status;
            data->second.is_waiting = false;
            //
            // std::cout << "Packet ACTION_EXECUTE_RESULT" << (uint)log->action_execute_result.status << std::endl;

            if (action_callback) action_callback(&data->second);
        } else {
            std::cout << "Unexpected ACTION_EXECUTE_RESULT" << std::endl;
        }
    }

    if (log->type == LogicalPacketType::ACTION_RESPONSE) {
        if (auto data = action_responses.find(log->action_response.request_id); data != action_responses.end()) {
            assert(data->second.is_waiting);

            auto payload_size = size - LOG_PACKET_SIZE(action_response) + 1;
            data->second.status = log->action_response.status;
            data->second.payload = std::vector<ubyte>(payload_size);

            std::memcpy(&data->second.payload.front(), log->action_response.payload, payload_size);
            data->second.is_waiting = false;

            if (action_callback) action_callback(&data->second);
        } else {
            std::cout << "Unexpected ACTION_RESPONSE" << std::endl;
        }
    }

    if (log->type == LogicalPacketType::SUBSCRIPTION_DONE) {
        if (auto data = subscriptions.find(log->subscription_callback.id); data != subscriptions.end()) {
            std::cout << "SUBSCRIPTION_DONE id: " << log->subscription_callback.id << std::endl;

            if (data->second.done_callback)
                data->second.done_callback(&data->second);

            if (data->second.is_renew)
                subscribe_action(data->second);
        } else {
            std::cout << "Unexpected SUBSCRIPTION_DONE" << std::endl;
        }
    }

    if (log->type == LogicalPacketType::SUBSCRIPTION_CALLBACK) {
        std::cout << "Income subscription with id: " << log->subscription_callback.id << " = "
                << net_load(log->subscription_callback.id) << std::endl;

        if (auto data = subscriptions.find(log->subscription_callback.id); data != subscriptions.end()) {
            auto payload_size = size - LOG_PACKET_SIZE(subscription_callback) + 1;
            auto result = std::vector<ubyte>(payload_size);

            std::memcpy(&result.front(), log->subscription_callback.payload, payload_size);

            data->second.callback(&data->second, std::move(result));
        } else {
            std::cout << "Unexpected SUBSCRIPTION_CALLBACK" << std::endl;
        }
    }
}

PhyDeviceInfo*PhyDeviceManager::lookup(MeshProto::far_addr_t addr) {
    if (known_devices.find(addr) == known_devices.end()) {
        printf("[khawasu] Not found phy (%d)\n", addr);
        return nullptr;
    }

    return &known_devices.at(addr);
}

LogDeviceInfo*PhyDeviceManager::lookup_log(MeshProto::far_addr_t addr, ushort port) {
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

    for (auto &pair: dev->actions_map) {
        std::cout << "ACTION " << pair.second.name << std::endl;
        if (strcmp(pair.second.name, name) == 0)
            return &pair;
    }

    std::cout << "NOT FOUND ACTION " << std::endl;

    return nullptr;
}

int
PhyDeviceManager::execute_action_async(MeshProto::far_addr_t addr, ushort port, ushort action_id,
                                       std::vector<ubyte> buffer,
                                       bool require_status) {
    ubyte req_id = action_responses_counter++;

    if (require_status)
        action_responses.emplace(req_id,
                                 ActionResponse(req_id, ActionResponseType::EXECUTE, addr, port,
                                                Os::get_microseconds()));

    auto log = device_manager->alloc_logical_packet_ptr(LogicalAddress(addr, port), controller.self_port, buffer.size(),
                                                        OverlayProtoType::UNRELIABLE,
                                                        LogicalPacketType::ACTION_EXECUTE);
    net_store(log.ptr()->action_execute.request_id, req_id);
    net_store(log.ptr()->action_execute.action_id, action_id);

    if (require_status)
        net_store(log.ptr()->action_execute.flags, ActionExecuteFlags::REQUIRE_STATUS_RESPONSE);

    net_memcpy((ubyte *) log.ptr()->action_execute.payload, buffer.data(), buffer.size());
    device_manager->finish_ptr(log);

    std::cout << "Sent EXECUTE ACTION PACKET" << std::endl;

    return req_id;
}

int PhyDeviceManager::execute_action_async(MeshProto::far_addr_t addr, ushort port, const char* action_name,
                                           std::vector<ubyte> buffer, bool require_status) {
    auto action = lookup_action(addr, port, action_name);

    // Place error
    if (action == nullptr) {
        ubyte req_id = action_responses_counter++;


        std::cout << "REQ_ID = " << (uint) req_id << std::endl;

        auto resp = ActionResponse(req_id, ActionResponseType::EXECUTE, addr, port, Os::get_microseconds());
        resp.is_waiting = false;
        resp.status = ActionExecuteStatus::UNKNOWN;

        std::thread t1([&] {
            if (action_callback)
                action_callback(&resp);
        });
        t1.join();

        return req_id;
    }

    return execute_action_async(addr, port, action->first, std::move(buffer), require_status);
}

ActionExecuteStatus PhyDeviceManager::execute_action(MeshProto::far_addr_t addr, ushort port, ushort action_id,
                                                     std::vector<ubyte> buffer, bool require_status) {
    uint req_id = execute_action_async(addr, port, action_id, buffer, require_status);

    if (!require_status)
        return ActionExecuteStatus::UNKNOWN;


    auto start_time = Os::get_microseconds();
    auto status = ActionExecuteStatus::TIMEOUT;
    ActionResponse* result = nullptr;
    while (status == ActionExecuteStatus::TIMEOUT &&
           Os::get_microseconds() - start_time < DEV_MAN_RESPONSE_TIMEOUT) {
        auto it = action_responses.find(req_id);
        if (it != action_responses.end() && !it->second.is_waiting) {
            result = new ActionResponse(it->second);
            result->time_received = Os::get_microseconds();
            status = result->status;

            std::cout << "[execute_action] Time spend: " << (result->time_received - result->time_sent) / 1'000.0
                    << " ms" << std::endl;

            action_responses.erase(it);
        }

        Os::yield_non_starving();
    }

    if (action_callback) action_callback(result);

    delete result;

    return status;
}

ActionExecuteStatus PhyDeviceManager::execute_action(MeshProto::far_addr_t addr, ushort port, const char* action_name,
                                                     std::vector<ubyte> buffer, bool require_status) {
    auto action = lookup_action(addr, port, action_name);
    if (action == nullptr)
        return ActionExecuteStatus::ACTION_NOT_FOUND;

    return execute_action(addr, port, action->first, std::move(buffer), require_status);
}

int
PhyDeviceManager::fetch_action_async(MeshProto::far_addr_t addr, ushort port, ushort action_id,
                                     std::vector<ubyte> buffer) {
    ubyte req_id = action_responses_counter++;

    action_responses.emplace(req_id, ActionResponse(req_id, ActionResponseType::FETCH, addr, port,
                                                    Os::get_microseconds()));

    auto log = device_manager->alloc_logical_packet_ptr(LogicalAddress(addr, port), controller.self_port, buffer.size(),
                                                        OverlayProtoType::UNRELIABLE,
                                                        LogicalPacketType::ACTION_FETCH);
    net_store(log.ptr()->action_fetch.request_id, req_id);
    net_store(log.ptr()->action_fetch.action_id, action_id);
    net_memcpy((ubyte *) log.ptr()->action_fetch.payload, buffer.data(), buffer.size());
    device_manager->finish_ptr(log);

    return req_id;
}

int PhyDeviceManager::fetch_action_async(MeshProto::far_addr_t addr, ushort port, const char* action_name,
                                         std::vector<ubyte> buffer) {
    auto action = lookup_action(addr, port, action_name);
    if (action == nullptr)
        return ERROR;

    return fetch_action_async(addr, port, action->first, std::move(buffer));
}

ActionResponse*
PhyDeviceManager::fetch_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer) {
    auto req_id = fetch_action_async(addr, port, action_id, buffer);

    auto start_time = Os::get_microseconds();
    ActionResponse* result = nullptr;
    while (result == nullptr &&
           Os::get_microseconds() - start_time < DEV_MAN_RESPONSE_TIMEOUT) {
        auto it = action_responses.find(req_id);
        if (it != action_responses.end() && !it->second.is_waiting) {
            result = new ActionResponse(it->second);
            result->time_received = Os::get_microseconds();

            std::cout << "[fetch_action] Time spend: " << (result->time_received - result->time_sent) / 1'000.0 << " ms"
                    << std::endl;

            action_responses.erase(it);
        }

        Os::yield_non_starving();
    }

    if (action_callback) action_callback(result);

    return result;
}

ActionResponse*
PhyDeviceManager::fetch_action(MeshProto::far_addr_t addr, ushort port, const char* action_name,
                               std::vector<ubyte> buffer) {
    auto action = lookup_action(addr, port, action_name);
    if (action == nullptr)
        return nullptr;

    return fetch_action(addr, port, action->first, std::move(buffer));
}

void PhyDeviceManager::subscribe_action(const ActionSubscriptionData &data) {
    assert(data.device);

    subscriptions.emplace(data.id, data);

    auto log = device_manager->alloc_logical_packet_ptr(LogicalAddress(data.device->phy, data.device->log),
                                                        controller.self_port, 0,
                                                        OverlayProtoType::UNRELIABLE,
                                                        LogicalPacketType::SUBSCRIPTION_START);

    net_store(log.ptr()->subscription_start.id, data.id);
    net_store(log.ptr()->subscription_start.action_id, data.action_id);
    net_store(log.ptr()->subscription_start.duration, data.duration);
    net_store(log.ptr()->subscription_start.period, data.period);
    //net_memcpy((ubyte*) log.ptr()->subscription_start.info_payload, buffer.data(), buffer.size());

    device_manager->finish_ptr(log);

    std::cout << "Start subscribe " << data.id << std::endl;
}

void PhyDeviceManager::subscribe_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, ushort duration,
                                        uint period, std::vector<ubyte> buffer,
                                        SubscriptionCallback callback,
                                        SubscriptionDoneCallback done_callback,
                                        bool is_renew) {
    auto sub_id = subscription_counter++;

    ActionSubscriptionData data;
    data.id = sub_id;
    data.action_id = action_id;
    data.period = period;
    data.duration = duration;
    data.callback = std::move(callback);
    data.device = lookup_log(addr, port);
    data.is_renew = is_renew;

    if (data.device == nullptr) {
        return;
    }


    subscribe_action(data);
}

void
PhyDeviceManager::subscribe_action(MeshProto::far_addr_t addr, ushort port, const char* action_name, ushort duration,
                                   uint period, std::vector<ubyte> buffer,
                                   SubscriptionCallback callback,
                                   SubscriptionDoneCallback done_callback,
                                   bool is_renew) {
    auto action = lookup_action(addr, port, action_name);
    if (action == nullptr)
        return;

    subscribe_action(addr, port, action->first, duration, period, std::move(buffer), std::move(callback),
                     std::move(done_callback), is_renew);
}

PhyDeviceManager::PhyDeviceManager(LogicalDeviceManager* device_manager_)
    : device_manager(device_manager_),
      controller(DeviceController(device_manager_, DEV_CONTROLLER_NAME, gen_uniq_port())) {
}

void PhyDeviceManager::start() {
    device_manager->add_device(&controller);
    //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}
