#include <stdint.h>

void misc_get_netif_mac_wifi(uint8_t* puMac_out);

#define TASKWISE_LOGF(log_iface_function, tag, task_name, fmt, ...) \
    log_iface_function(tag, "[%s] " fmt, task_name, ##__VA_ARGS__)

#define TASKWISE_LOG(log_iface_function, tag, task_name, msg) \
    log_iface_function(tag, "[%s] %s", task_name, msg)
