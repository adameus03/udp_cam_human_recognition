#include "camau_controller.h"
#include "analyser.h"
#include "server_communications.h"
#include "esp_log.h"

static const char *TAG = "CAMAU_CONTROLLER";

void camau_controller_init(void) {
    ESP_LOGI(TAG, "Initialising camau controller");
    analyser_init();
}

void camau_controller_run(void) {
    ESP_LOGI(TAG, "Running camau controller");
    //begin_udp_stream();//
    analyser_run();
}

void x_analyser_perform_action(int width, int height, int format, uint8_t * data, size_t len) {
    ESP_LOGI(TAG, "Performing action");
    begin_udp_stream();
    transmit_udp(data, len);
    end_udp_stream();
}

unsigned int frame_index = 0;
void x_on_udp_transmission_end() {
    ESP_LOGI(TAG, "Streaming frame %d complete", frame_index);
    frame_index++;
}