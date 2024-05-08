#include "analyser.h"
#include "camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sau_heap_debug.h"

static const char * TAG = "analyser";

//static SemaphoreHandle_t ;;;

void analyser_init() {
    ESP_LOGI(TAG, "Initialising analyser");

}

void __analyser_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting analyser task");
    //call camera_capture() in a loop to capture images continuously and process them
    while(1) {
        camera_capture();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void analyser_run(uint32_t usStackDepth) {
    ESP_LOGI(TAG, "Running analyser");
    camera_init(FRAMESIZE_UXGA, 12);
    ESP_LOGI(TAG, "Returned from camera_init");


    sau_heap_debug_info();
    ESP_LOGI(TAG, "Going to allocate stack for task analyser_task, needed heap memory block: %lu", usStackDepth);

    // Create the analyser task on core 1
    xTaskCreatePinnedToCore(__analyser_task, "analyser_task", usStackDepth, NULL, 5, NULL, 1); // [TODO] Really?
    //__analyser_task(NULL);
    sau_heap_debug_info();
}


//unsigned int testing_index = 0;
//unsigned char analyser_in_excitement = 0;

static analyser_operation_mode_t operation_mode = ANALYSER_OPERATION_MODE_ANALYSIS;

void analyser_set_operation_mode(analyser_operation_mode_t mode) {
    operation_mode = mode;
}


/**
 * @brief This function is called when a camera frame is captured
*/
void x_camera_process_image(int width, int height, int format, uint8_t** ppuDataBufAddr, size_t len) {
    switch (operation_mode) {
        case ANALYSER_OPERATION_MODE_ANALYSIS:
            //printf("!");
            break;
        case ANALYSER_OPERATION_MODE_UNCONDITIONAL_STREAM:
            x_analyser_perform_action(width, height, format, ppuDataBufAddr, len);
            break;
        default:
            ESP_LOGE(TAG, "Unknown analyser operation mode!");
            break;
    }


    /*ESP_LOGD(TAG, "Testing index: %d", testing_index);
    if (analyser_in_excitement) {
        ESP_LOGI(TAG, "In excitement, skipping image processing");
        x_analyser_perform_action(width, height, format, ppuDataBufAddr, len);
        if (testing_index == 1) {//9
            testing_index = 0;
            analyser_in_excitement = 0;
            return;
        }
    } else {
        ESP_LOGI(TAG, "Processing image");
        if(testing_index == 0) {//99
            ESP_LOGD(TAG, "Action triggered");
            testing_index = 0;
            analyser_in_excitement = 1;
            x_analyser_perform_action(width, height, format, ppuDataBufAddr, len);
        }
    }
    testing_index++;*/
}

