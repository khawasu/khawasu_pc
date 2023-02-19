#include "c_api.h"
#include <protocols/logical_proto.h>
#include <protocols/overlay_proto.h>
#include <cstring>
#include <mesh_controller.h>
#include <logical_device_manager.h>
#include "interfaces/mesh_socket_interface.h"
#include "phy_device_manager.h"
#include "logical_callback_device.h"

using namespace LogicalProto;
using namespace OverlayProto;


typedef struct  {
    LogicalDeviceManager* device_manager;
    PhyDeviceManager* phy_device_manager;
    // fixme: make initialization in construct
    NEW_DEVICE_CALLBACK new_device_callback;
    ACTION_EXECUTE_CALLBACK action_execute_callback;
    ACTION_FETCH_CALLBACK action_fetch_callback;
} CONTEXT;


// TODO: Make codegen
extern "C" const char* dev_class_to_string(unsigned int dev_class) {
    switch ((LogicalProto::DeviceClassEnum) dev_class) {
        case LogicalProto::DeviceClassEnum::UNKNOWN:
            return "UNKNOWN";
        case LogicalProto::DeviceClassEnum::BUTTON:
            return "BUTTON";
        case LogicalProto::DeviceClassEnum::RELAY:
            return "RELAY";
        case LogicalProto::DeviceClassEnum::TEMPERATURE_SENSOR:
            return "TEMPERATURE_SENSOR";
        case LogicalProto::DeviceClassEnum::TEMP_HUM_SENSOR:
            return "TEMP_HUM_SENSOR";
        case LogicalProto::DeviceClassEnum::CONTROLLER:
            return "CONTROLLER";
        case LogicalProto::DeviceClassEnum::PC2LOGICAL_ADAPTER:
            return "PC2LOGICAL_ADAPTER";
        case LogicalProto::DeviceClassEnum::LUA_INTERPRETER:
            return "LUA_INTERPRETER";
        case LogicalProto::DeviceClassEnum::LED_1_DIM:
            return "LED_1_DIM";
        case LogicalProto::DeviceClassEnum::LED_2_DIM:
            return "LED_2_DIM";
        case LogicalProto::DeviceClassEnum::HW_ACCESSOR:
            return "HW_ACCESSOR";
        case LogicalProto::DeviceClassEnum::PY_INTERPRETER:
            return "PY_INTERPRETER";
        case LogicalProto::DeviceClassEnum::STRING_NAME:
            return "STRING_NAME";
    }

    return nullptr;
}

// TODO: Make codegen
extern "C" unsigned int dev_class_to_int(const char* dev_class) {
    if (strcmp(dev_class, "UNKNOWN") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::UNKNOWN;

    if (strcmp(dev_class, "BUTTON") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::BUTTON;

    if (strcmp(dev_class, "RELAY") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::RELAY;

    if (strcmp(dev_class, "TEMPERATURE_SENSOR") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::TEMPERATURE_SENSOR;

    if (strcmp(dev_class, "TEMP_HUM_SENSOR") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::TEMP_HUM_SENSOR;

    if (strcmp(dev_class, "CONTROLLER") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::CONTROLLER;

    if (strcmp(dev_class, "PC2LOGICAL_ADAPTER") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::PC2LOGICAL_ADAPTER;

    if (strcmp(dev_class, "LUA_INTERPRETER") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::LUA_INTERPRETER;

    if (strcmp(dev_class, "LED_1_DIM") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::LED_1_DIM;

    if (strcmp(dev_class, "LED_2_DIM") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::LED_2_DIM;

    if (strcmp(dev_class, "HW_ACCESSOR") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::HW_ACCESSOR;

    if (strcmp(dev_class, "PY_INTERPRETER") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::PY_INTERPRETER;

    if (strcmp(dev_class, "STRING_NAME") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::STRING_NAME;

    return 0;
}

// TODO: Make codegen
extern "C" const char* action_type_to_string(unsigned int type) {
    switch ((ActionType) type) {
        case ActionType::UNKNOWN:
            return "UNKNOWN";
        case ActionType::IMMEDIATE:
            return "IMMEDIATE";
        case ActionType::TOGGLE:
            return "TOGGLE";
        case ActionType::RANGE:
            return "RANGE";
        case ActionType::LABEL:
            return "LABEL";
        case ActionType::TEMPERATURE:
            return "TEMPERATURE";
        case ActionType::HUMIDITY:
            return "HUMIDITY";
        case ActionType::TIME_DELTA:
            return "TIME_DELTA";
        case ActionType::TIME:
            return "TIME";
    }

    return nullptr;
}

// TODO: Make codegen
extern "C" unsigned int action_type_to_int(const char* type) {
    if (strcmp(type, "UNKNOWN") == 0)
        return (unsigned int) LogicalProto::DeviceClassEnum::UNKNOWN;

    if (strcmp(type, "IMMEDIATE") == 0)
        return (unsigned int) ActionType::IMMEDIATE;

    if (strcmp(type, "TOGGLE") == 0)
        return (unsigned int) ActionType::TOGGLE;

    if (strcmp(type, "RANGE") == 0)
        return (unsigned int) ActionType::RANGE;

    if (strcmp(type, "LABEL") == 0)
        return (unsigned int) ActionType::LABEL;

    if (strcmp(type, "TEMPERATURE") == 0)
        return (unsigned int) ActionType::TEMPERATURE;

    if (strcmp(type, "HUMIDITY") == 0)
        return (unsigned int) ActionType::HUMIDITY;

    if (strcmp(type, "TIME_DELTA") == 0)
        return (unsigned int) ActionType::TIME_DELTA;

    if (strcmp(type, "TIME") == 0)
        return (unsigned int) ActionType::TIME;

    return 0;
}

inline void mesh_packet_handler(MeshProto::far_addr_t src_phy, const ubyte* data, ushort size, void* user_data) {
    if (size < OVL_PACKET_SIZE(type))
        return;

    auto ovl = (OverlayPacket*) data;
    LogicalPacket* log;
    switch (ovl->type) {
        case OverlayProtoType::RELIABLE: {
            if (size < OVL_PACKET_SIZE(reliable.data))
                return;
            log = (LogicalPacket*) ovl->reliable.data;
            break;
        }
        case OverlayProtoType::UNRELIABLE: {
            if (size < OVL_PACKET_SIZE(unreliable.data))
                return;
            log = (LogicalPacket*) ovl->unreliable.data;
            break;
        }
        default:
            return;
    }

    printf("[khawasu] New packet from %d to %d! %d bytes, type %hhu\n", log->src_addr, log->dst_addr, size, log->type);
    fflush(stdout);

    ((PhyDeviceManager*) ((CONTEXT*) user_data)->phy_device_manager)->handle_packet(src_phy, log, size);
    ((PhyDeviceManager*) ((CONTEXT*) user_data)->phy_device_manager)->device_manager->dispatch_packet(log, size,
                                                                                                      src_phy);
}


MeshController* g_fresh_mesh = nullptr;

extern "C" void* init_ctx(const char* network_name, unsigned int far_addr, const char* network_password,
                          const char* peer_ip_addr, unsigned short peer_port) {
    auto context = new CONTEXT();
    context->device_manager = new LogicalDeviceManager();

    auto controller = new MeshController(network_name, far_addr);
    g_fresh_mesh = controller;
    controller->set_psk_password(network_password);
    controller->user_stream_handler = [context](MeshProto::far_addr_t src_addr, const ubyte* data, ushort size) {
        printf("[%d] user_stream_handler\n", g_fresh_mesh->self_addr);
        mesh_packet_handler(src_addr, data, size, context);
    };

    ((PhyDeviceManager*) context->phy_device_manager)->new_device_callback = [context](const LogDeviceInfo& dev) {
        if (context->new_device_callback == nullptr)
            return;

        std::vector<ACTION> actions;
        actions.reserve(dev.actions_map.size());

        for (auto kv: dev.actions_map) {
            ACTION act;
            act.name = new char[kv.second.length + 1];
            std::memcpy(act.name, kv.second.name, kv.second.length);
            act.name[kv.second.length] = '\0';
            act.type = (unsigned int) kv.second.type;

            actions.push_back(act);
        }

        context->new_device_callback(dev.phy, dev.log, dev.name.c_str(), (unsigned int) dev.dev_class, actions.data(),
                                     actions.size());

    };

    MeshSocketInterface socket_interface(peer_ip_addr, peer_port, false);
    controller->add_interface(&socket_interface);

    OverlayPacketBuilder::log_ovl_packet_alloc =
            new PoolMemoryAllocator<LOG_PACKET_POOL_ALLOC_PART_SIZE, LOG_PACKET_POOL_ALLOC_COUNT>();

    context->phy_device_manager = new PhyDeviceManager(context->device_manager);

    return context;
}

extern "C" void register_callback_new_device(void* ctx, NEW_DEVICE_CALLBACK callback) {
    if (ctx != nullptr)
        ((CONTEXT*) ctx)->new_device_callback = callback;
}

extern "C" void register_callback_action_execute(void* ctx, ACTION_EXECUTE_CALLBACK callback) {
    if (ctx != nullptr)
        ((CONTEXT*) ctx)->action_execute_callback = callback;

}

extern "C" void register_callback_action_fetch(void* ctx, ACTION_FETCH_CALLBACK callback) {
    if (ctx != nullptr)
        ((CONTEXT*) ctx)->action_fetch_callback = callback;
}


struct ActionHandlerData {
    CONTEXT* ctx;
    LogicalCallbackDevice* dev;
};

static std::vector<ubyte> action_handler_get(const char* name, ActionType type, const ubyte* data, uint size,
                                             LogicalAddress addr, ubyte req_id, void* custom_data) {
    auto handler_data = ((ActionHandlerData*) custom_data);
    if (handler_data->ctx->action_fetch_callback == nullptr)
        return {};

    auto out_size = new size_t(size);
    auto result = (ubyte*) handler_data->ctx->action_fetch_callback(addr.phy, addr.log, handler_data->dev->self_port,
                                                                    name, (unsigned int) type, (void*) data, out_size);

    return {result, result + *out_size};
}

static ActionExecuteStatus action_handler_set(const char* name, ActionType type, const ubyte* data, uint size,
                                              LogicalAddress addr, void* custom_data) {
    auto handler_data = ((ActionHandlerData*) custom_data);
    if (handler_data->ctx->action_execute_callback == nullptr)
        return ActionExecuteStatus::UNKNOWN;

    return (ActionExecuteStatus) handler_data->ctx->action_execute_callback(addr.phy, addr.log,
                                                                            handler_data->dev->self_port, name,
                                                                            (unsigned int) type, (void*) data);
}

extern "C" void create_device(void* ctx, unsigned int phy_addr, unsigned short log_addr, const char* name,
                              unsigned int dev_class, ACTION* actions, size_t size) {
    auto phy_device_manager = ((CONTEXT*) ctx)->phy_device_manager;

    auto dev = new LogicalCallbackDevice(phy_device_manager->device_manager, name, log_addr);
    dev->dev_class = (DeviceClassEnum) dev_class;
    auto data = new ActionHandlerData{(CONTEXT*) ctx, dev};

    for (int i = 0; i < size; i++) {
        auto act = actions[i];
        dev->add_action({act.name, (ActionType) act.type}, action_handler_get, action_handler_set, data);
    }

    phy_device_manager->device_manager->add_device(dev);
}

extern "C" unsigned int execute_action(void* ctx, unsigned int dest_phy, unsigned short dest_log,
                                       unsigned short src_log, const char* name, void* data, size_t data_size,
                                       int require_result) {
    auto man = ((CONTEXT*) ctx)->phy_device_manager;
    auto data_ = (ubyte*) data;

    return (unsigned int) man->execute_action(dest_phy, dest_log, name, {data_, data_ + data_size}, require_result);
}

extern "C" void* fetch_action(void* ctx, unsigned int dest_phy, unsigned short dest_log, unsigned short src_log, const char* name,
                   void* data, size_t data_size, size_t* out_size) {
    auto man = ((CONTEXT*) ctx)->phy_device_manager;
    auto data_ = (ubyte*) data;

    // TODO: make specify src_phy
    auto result = man->fetch_action(dest_phy, dest_log, name, {data_, data_ + data_size});
    *out_size = result->size();

    return result->data();
}

extern "C" void remove_device(void* ctx, unsigned short dev_port) {
    auto man = ((CONTEXT*) ctx)->phy_device_manager;
    man->device_manager->remove_device(man->device_manager->lookup_device(dev_port));
}

extern "C" void deinit_ctx(void* ctx) {
    auto context = (CONTEXT*) ctx;
    delete context->phy_device_manager->device_manager;
    delete context->phy_device_manager;
}

