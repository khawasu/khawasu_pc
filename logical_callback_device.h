#pragma once

#include <logical_device.h>

using namespace LogicalProto;

struct DeviceCallbackApiAction {
    std::vector<ubyte> (* handler_get)(const char*, ActionType, const ubyte*, uint, LogicalAddress, ubyte, void*);

    ActionExecuteStatus (* handler_set)(const char*, ActionType, const ubyte*, uint, LogicalAddress, void*);

    void* custom_data = nullptr;
};

class LogicalCallbackDevice : public LogicalDevice {
public:
    LogicalProto::DeviceClassEnum dev_class;
    std::vector<DeviceApiAction> api_actions;
    std::unordered_map<std::string, DeviceCallbackApiAction> api_callbacks;

    OVERRIDE_DEV_CLASS(dev_class);

    OVERRIDE_ATTRIBS({ "vendor", "virtual" })

    std::pair<DeviceApiAction*, ubyte> get_api_actions() override {
        return {(DeviceApiAction*) api_actions.data(), api_actions.size()};
    }

    uint get_action_id(const char* name, int string_len);

    explicit LogicalCallbackDevice(LogicalDeviceManager* manager, const char* name_, ushort port_)
            : LogicalDevice(manager, name_, port_), dev_class(LogicalProto::DeviceClassEnum::UNKNOWN) {

    }

    void add_action(DeviceApiAction data,
                    std::vector<ubyte> (* handler_get)(const char*, ActionType, const ubyte*, uint, LogicalAddress,
                                                       ubyte, void*),
                    ActionExecuteStatus (* handler_set)(const char*, ActionType, const ubyte*, uint, LogicalAddress,
                                                        void*),
                    void* custom_data);

    void on_action_get(int action_id, const ubyte* data, uint size, LogicalAddress addr, ubyte request_id) override;

    ActionExecuteStatus on_action_set(int action_id, const ubyte* data, uint size, LogicalAddress addr) override;


};