#pragma once

#include <new>
#include <random>
#include <unordered_map>
#include <climits>
#include <mesh_protocol.h>
#include "device_info.h"
#include <net_utils.h>
#include <preserved_property.h>

#include "logical_device.h"
#include "logical_device_manager.h"

#include "messages.cpp"

#define DEV_CONTROLLER_NAME "Controller"
#define DEV_MAN_RANGE_MIN 6000
#define DEV_MAN_RANGE_MAX 7000
#define DEV_CONTROLLER_PORT 5999
#define DEV_MAN_RESPONSE_TIMEOUT 5'000'000

struct PowerLevel;

class DeviceController : public LogicalDevice {
public:
    PROPERTY(ubyte, current_level, 0);

    explicit DeviceController(LogicalDeviceManager* manager, const char* name_, ushort port_)
        : LogicalDevice(manager, name_, port_) {
    }

    ActionExecuteStatus on_action_set(int action_id, const ubyte* data, uint size, LogicalAddress addr) override {
        if (action_id == get_action_id("power_level")) {
            if (size >= PowerLevel::get_alloc_size()) {
                auto message = (PowerLevel *) data;
                current_level = message->value();

                return ActionExecuteStatus::SUCCESS;
            }
            return ActionExecuteStatus::ARGUMENTS_ERROR;
        }

        return ActionExecuteStatus::ACTION_NOT_FOUND;
    }

    void on_action_get(int action_id, const ubyte* data, uint size, LogicalAddress addr, ubyte request_id) override {
        if (action_id == get_action_id("power_level")) {
            auto log = dev_manager->alloc_logical_packet_ptr(addr, self_port,
                                                             PowerLevel::get_alloc_size(),
                                                             OverlayProto::OverlayProtoType::UNRELIABLE,
                                                             LogicalPacketType::ACTION_RESPONSE);
            PowerLevel::Initialize((std::int8_t *) &log.ptr()->action_response.payload[0]);

            auto message = (PowerLevel *) &log.ptr()->action_response.payload[0];
            message->value() = *current_level;

            net_store(log.ptr()->action_response.request_id, request_id);
            net_store(log.ptr()->action_response.status, ActionExecuteStatus::SUCCESS);

            dev_manager->finish_ptr(log);
        }
    }

    OVERRIDE_DEV_CLASS(DeviceClassEnum::CONTROLLER);
    OVERRIDE_ACTIONS({ActionType::RANGE, "power_level"})
};

typedef std::function<void(const LogDeviceInfo &)> NewDeviceCallback;

typedef std::function<void(const ActionResponse*)> ActionCallback;

struct ActionSubscriptionData;

typedef std::function<void(const ActionSubscriptionData*, std::vector<ubyte>)> SubscriptionCallback;
typedef std::function<void(const ActionSubscriptionData*)> SubscriptionDoneCallback;


struct ActionSubscriptionData {
    LogDeviceInfo* device;

    uint id;
    ushort action_id;
    ushort duration;
    uint period;

    bool is_renew = false;

    SubscriptionCallback callback;
    SubscriptionDoneCallback done_callback;
};

class PhyDeviceManager {
public:
    std::unordered_map<MeshProto::far_addr_t, PhyDeviceInfo> known_devices;
    std::unordered_map<uint, ActionSubscriptionData> subscriptions;

    LogicalDeviceManager* device_manager;
    DeviceController controller;

    std::unordered_map<ubyte, ActionResponse> action_responses;

    std::atomic<ubyte> action_responses_counter = 0;
    uint subscription_counter = 0;

    const int ERROR = 0;

    NewDeviceCallback new_device_callback;
    ActionCallback action_callback;


    static MeshProto::far_addr_t get_self_addr();

    ushort gen_uniq_port();

    void send_hello();

    void handle_packet(MeshProto::far_addr_t src_phy, LogicalPacket* log, ushort size);

    PhyDeviceInfo*lookup(MeshProto::far_addr_t addr);

    LogDeviceInfo*lookup_log(MeshProto::far_addr_t addr, ushort port);

    std::pair<const int, DeviceApiAction>*lookup_action(MeshProto::far_addr_t addr, ushort port, const char* name);

    int
    execute_action_async(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer,
                         bool require_status);

    int
    execute_action_async(MeshProto::far_addr_t addr, ushort port, const char* action_name, std::vector<ubyte> buffer,
                         bool require_status);

    ActionExecuteStatus execute_action(MeshProto::far_addr_t addr, ushort port, ushort action_id,
                                       std::vector<ubyte> buffer, bool require_status);

    ActionExecuteStatus execute_action(MeshProto::far_addr_t addr, ushort port,
                                       const char* action_name,
                                       std::vector<ubyte> buffer, bool require_status);

    int
    fetch_action_async(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer);

    int
    fetch_action_async(MeshProto::far_addr_t addr, ushort port, const char* action_name, std::vector<ubyte> buffer);

    ActionResponse*
    fetch_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, std::vector<ubyte> buffer);

    ActionResponse*
    fetch_action(MeshProto::far_addr_t addr, ushort port, const char* action_name, std::vector<ubyte> buffer);

    void subscribe_action(const ActionSubscriptionData &data);

    void subscribe_action(MeshProto::far_addr_t addr, ushort port, ushort action_id, ushort duration, uint period,
                          std::vector<ubyte> buffer,
                          SubscriptionCallback callback,
                          SubscriptionDoneCallback done_callback = nullptr,
                          bool is_renew = false);

    void subscribe_action(MeshProto::far_addr_t addr, ushort port, const char* action_name, ushort duration,
                          uint period, std::vector<ubyte> buffer,
                          SubscriptionCallback callback,
                          SubscriptionDoneCallback done_callback = nullptr,
                          bool is_renew = false);

    explicit PhyDeviceManager(LogicalDeviceManager* device_manager_);

    void start();
};
