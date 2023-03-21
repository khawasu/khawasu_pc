#include <iostream>
#include <mesh_controller.h>
#include <platform/p2p/win32_p2p.h>
#include <logical_device_manager.h>
#include <preserved_property.h>
#include "socket/socket_base.h"
#include "interfaces/mesh_socket_interface.h"
#include "arg_parser.h"

using namespace LogicalProto;
using namespace OverlayProto;

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
        default: return;
    }

    printf("[khawasu] New packet from %d to %d! %d bytes, type %hhu\n", log->src_addr, log->dst_addr, size, log->type);
    fflush(stdout);

    ((LogicalDeviceManager*)user_data)->dispatch_packet(log, size, src_phy);
}

class DeviceRelay : public LogicalDevice
{
public:
    PROPERTY(ubyte, current_level, (ubyte)0);

    DeviceRelay(LogicalDeviceManager* manager_, const char* name_, ushort port_)
            : LogicalDevice(manager_, name_, port_) {
        std::cout << *current_level << std::endl;
    }


    ActionExecuteStatus on_action_set(int action_id, const ubyte* data, uint size, LogicalAddress addr) override {
        std::cout << "set" << std::endl;
        if(action_id == get_action_id("power_state")){
            if (size < 1)
                return ActionExecuteStatus::ARGUMENTS_ERROR;



            return ActionExecuteStatus::SUCCESS;
        }

        return ActionExecuteStatus::ACTION_NOT_FOUND;
    }

    void on_action_get(int action_id, const ubyte* data, uint size, LogicalAddress addr, ubyte request_id) override {
        std::cout << "get" << std::endl;
        if (action_id == get_action_id("power_state")) {
            auto log = dev_manager->alloc_logical_packet_ptr(addr, self_port, 1,
                                                             OverlayProtoType::UNRELIABLE,
                                                             LogicalPacketType::ACTION_RESPONSE);
            log.ptr()->action_response.payload[0] = 1;
            log.ptr()->action_response.request_id = request_id;
            dev_manager->finish_ptr(log);
        }
    }

    void on_subscription_timer_update(LogicalAddress addr, uint sub_id, ushort action_id, const void* format) override {
        std::cout << "sub" << std::endl;
        if (action_id == get_action_id("power_state")) {
            auto log = dev_manager->alloc_logical_packet_ptr(addr, self_port, 1,
                                                             OverlayProtoType::UNRELIABLE,
                                                             LogicalPacketType::SUBSCRIPTION_CALLBACK);
            log.ptr()->action_response.payload[0] = 1;
            dev_manager->finish_ptr(log);
        }
    }

    OVERRIDE_DEV_CLASS(DeviceClassEnum::RELAY)

    OVERRIDE_ACTIONS({ActionType::TOGGLE, "power_state"})
};

int main(int argc, char* argv[]) {
    ArgParser argParser;
    argParser.process(argc, argv);

    LogicalDeviceManager manager;

    auto controller = new MeshController(argParser.network_name.c_str(), argParser.network_addr);
    g_fresh_mesh = controller;
    controller->set_psk_password(argParser.network_psk.c_str());
    controller->user_stream_handler = [&manager](MeshProto::far_addr_t src_addr, const ubyte* data, ushort size) {
        printf("[%d] user_stream_handler\n", g_fresh_mesh->self_addr);
        mesh_packet_handler(src_addr, data, size, &manager);
    };

    MeshSocketInterface socket_interface(argParser.hostname, argParser.port, false);
    controller->add_interface(&socket_interface);

    OverlayPacketBuilder::log_ovl_packet_alloc =
            new PoolMemoryAllocator<LOG_PACKET_POOL_ALLOC_PART_SIZE, LOG_PACKET_POOL_ALLOC_COUNT>();


    DeviceRelay r(&manager, "Wow", 1124);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    manager.add_device(&r);

    std::this_thread::sleep_for(std::chrono::days(2));

    return 0;
}
