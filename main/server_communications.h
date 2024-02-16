#include <stdint.h>
#include "esp_err.h"

esp_err_t begin_udp_stream();
void end_udp_stream();
esp_err_t transmit_udp(uint8_t* data, size_t len);
extern void x_on_udp_transmission_end();