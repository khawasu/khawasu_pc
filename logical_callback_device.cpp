#include "logical_callback_device.h"
#include <logical_device_manager.h>
#include <net_utils.h>

using namespace OverlayProto;

void LogicalCallbackDevice::add_action(DeviceApiAction data,
                                       std::vector<ubyte> (* handler_get)(const char*, ActionType, const ubyte*, uint,
                                                                          LogicalAddress, ubyte, void*),
                                       LogicalProto::ActionExecuteStatus (* handler_set)(const char*, ActionType,
                                                                                         const ubyte*, uint,
                                                                                         LogicalAddress, void*),
                                       void* custom_data) {
    api_actions.push_back(data);
    api_callbacks[data.name] = {handler_get, handler_set, custom_data};
}

uint LogicalCallbackDevice::get_action_id(const char* name, int string_len) {
    for (auto i = 0; i < api_actions.size(); ++i) {
        if (api_actions[i].length != string_len - 1)
            continue;

        int j;
        for (j = 0; j < string_len && api_actions[i].name[j] == name[j]; ++j);
        if (j >= string_len)
            return i;
    }
    return 254;
}

void
LogicalCallbackDevice::on_action_get(int action_id, const ubyte* data, uint size, LogicalAddress addr,
                                     ubyte request_id) {
    for (auto& [action_name, callbacks]: api_callbacks) {
        if (strcmp(api_actions[action_id].name, action_name.c_str()) == 0) {
            auto buffer = callbacks.handler_get(action_name.c_str(), api_actions[action_id].type, data, size, addr,
                                                request_id,
                                                callbacks.custom_data);

            auto log = dev_manager->alloc_logical_packet_ptr(addr, self_port, buffer.size(),
                                                             OverlayProtoType::UNRELIABLE,
                                                             LogicalPacketType::ACTION_RESPONSE);
            net_memcpy((ubyte*) log.ptr()->action_response.payload, buffer.data(), buffer.size());
            net_store(log.ptr()->action_response.request_id, request_id);
            dev_manager->finish_ptr(log);

            break;
        }

    }
}

ActionExecuteStatus
LogicalCallbackDevice::on_action_set(int action_id, const ubyte* data, uint size, LogicalAddress addr) {
    for (auto& [action_name, callbacks]: api_callbacks) {
        if (strcmp(api_actions[action_id].name, action_name.c_str()) == 0) {
            return callbacks.handler_set(action_name.c_str(), api_actions[action_id].type, data, size, addr,
                                         callbacks.custom_data);
        }
    }

    return ActionExecuteStatus::ACTION_NOT_FOUND;
}
