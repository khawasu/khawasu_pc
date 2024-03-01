//
// Created by ameharu on 06.02.2024.
//
#include <iostream>
#include "c_api.h"
#include "../../SmartHome/khawasu/khawasu_core/logical_device.h"
#include "../../SmartHome/thirdparties/fresh/main/platform/common_api.h"


int main() {
    auto ctx = init_ctx("dev network", 111, "1234", "127.0.0.1", 49152);

    register_callback_action_fetch_result(ctx, [](uint req_id, void *buffer, size_t size) {
        std::cout << "New action fetch result req_id=" << req_id << " size=" << size << " bytes" << std::endl;
    });

    register_callback_action_execute_result(ctx, [](uint req_id, unsigned int status) {
        std::cout << "New action fetch execute req_id=" << req_id << " status=" << status << std::endl;
    });

    register_callback_new_device(ctx, [](unsigned int phy_addr, unsigned short log_addr, const char *name, unsigned int dev_class, ACTION *, size_t size) {
        std::cout << "New device (" << dev_class << ") " << std::to_string(LogicalAddress(phy_addr, log_addr)) << " " << name << std::endl;
    });


    char buf[1];
    buf[0] = 0xFF;
    execute_action_async(ctx, 4344, 20, 0, "power_state", &buf, 1, false);

    while (true) {
        check_packets(ctx);
        Os::yield_non_starving();
    }


    deinit_ctx(ctx);
    return 0;
}
