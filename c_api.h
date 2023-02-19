#pragma once

#define FFI_SCOPE "KHAWASU"
#define FFI_LIBS "khawasu_capi"

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  {
    char* name;
    unsigned int type;
} ACTION;

typedef void (*NEW_DEVICE_CALLBACK) (unsigned int phy_addr, unsigned short log_addr,
                                     const char* name, unsigned int dev_class,
                                     ACTION*, size_t size);

typedef unsigned int (*ACTION_EXECUTE_CALLBACK) (unsigned int sender_phy, unsigned short sender_log,
                                                 unsigned short self_log, const char* action_name,
                                                 unsigned int action_type, void* data);

typedef void* (*ACTION_FETCH_CALLBACK) (unsigned int sender_phy, unsigned short sender_log,
                                        unsigned short self_log, const char* action_name,
                                        unsigned int action_type, void* data, size_t* out_size);

const char* dev_class_to_string(unsigned int dev_class);
unsigned int dev_class_to_int(const char* dev_class);

const char* action_type_to_string(unsigned int type);
unsigned int action_type_to_int(const char* type);


// Init khawasu_pc
void* init_ctx(const char* network_name, unsigned int far_addr, const char* network_password,
               const char* peer_ip_addr, unsigned short peer_port);

void register_callback_new_device(void* ctx, NEW_DEVICE_CALLBACK callback);

void create_device(void* ctx, unsigned int phy_addr, unsigned short log_addr,
                   const char* name, unsigned int dev_class,
                   ACTION*, size_t size);

// on_action(sender_phy: int, sender_log: int, self_log: int, act_name: str, act_type: int, data: ctypes.c_void_p) -> int
// callback returns status
void register_callback_action_execute(void* ctx, ACTION_EXECUTE_CALLBACK callback);
// callback returns data bytes
void register_callback_action_fetch(void* ctx, ACTION_FETCH_CALLBACK callback);

unsigned int execute_action(void* ctx, unsigned int dest_phy, unsigned short dest_log, unsigned short src_log,
                            const char* name, void* data, size_t data_size, int require_result);
// return data bytes
void* fetch_action(void* ctx, unsigned int dest_phy, unsigned short dest_log, unsigned short src_log,
                   const char* name, void* data, size_t data_size, size_t* out_size);


void remove_device(void* ctx, unsigned short dev_port);

void deinit_ctx(void* ctx);


#ifdef __cplusplus
}
#endif
