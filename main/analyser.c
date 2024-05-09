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
static uint8_t analysis_enabled = 0U;

void analyser_set_operation_mode(analyser_operation_mode_t mode) {
    operation_mode = mode;
}

//static uint8_t* analyser_old_frame = NULL;
static uint32_t analyser_old_frame_len = 0;
static uint8_t analyser_is_first_frame = 1U;
//static double analyser_threshold = 0.1;
//static uint32_t analyser_num_probes = 100;
static uint32_t analyse_lendiff_threshold = 200;

static const char* SPECIAL_ANALYSER_INDICATOR_TAG = "(THRESOLD) ";

/**
 * @brief Perform the analysis of the image and decide whether to trigger an action
*/
analyser_result_t analyser_analyse(uint8_t* puDataBuf, size_t len) {
    if (analyser_is_first_frame) {
        /*ESP_LOGI(TAG, "Allocating SPIRAM memory for analyser_old_frame...");
        analyser_old_frame = (uint8_t*) heap_caps_malloc(len, MALLOC_CAP_SPIRAM);*/
        analyser_old_frame_len = len;
        //ESP_LOGI(TAG, "SPIRAM memory allocation for analyser_old_frame succeeded");
        analyser_is_first_frame = 0U;
        return ANALYSER_RESULT_CALM;
    } else {
        /*if (analyser_old_frame_len != len) {
            ESP_LOGI(TAG, "Reallocating SPIRAM memory for analyser_old_frame...");
            analyser_old_frame = (uint8_t*) heap_caps_realloc((void*)analyser_old_frame, len, MALLOC_CAP_SPIRAM);
            analyser_old_frame_len = len;
            ESP_LOGI(TAG, "SPIRAM memory reallocation for analyser_old_frame succeeded");
            return ANALYSER_RESULT_CALM;
        } else {
            uint32_t sum = 0;
            uint32_t step = len / analyser_num_probes;
            for (uint32_t i = 0; i < len; i+=step) {
                sum += abs((int)*(puDataBuf + i) - (int)*(analyser_old_frame + i));
            }
            double indicator = ((double)sum) / (double)(len / step);
            return indicator > analyser_threshold ? ANALYSER_RESULT_TRIGGERED : ANALYSER_RESULT_CALM;

        }*/
        
        uint32_t lendiff = (uint32_t)abs(((int32_t)analyser_old_frame_len) - (int32_t)len);
        ESP_LOGI(SPECIAL_ANALYSER_INDICATOR_TAG, "[lendif is %d]", (int)lendiff);
        analyser_old_frame_len = len;
        if (lendiff > analyse_lendiff_threshold) {
            return ANALYSER_RESULT_TRIGGERED;
        } else {
            return ANALYSER_RESULT_CALM;
        }
    }
}

void analyser_shutdown() {
    analysis_enabled = 0U;
}
void analyser_wakeup() {
    analysis_enabled = 1U;
}

/**
 * @brief This function is called when a camera frame is captured
*/
void x_camera_process_image(int width, int height, int format, uint8_t** ppuDataBufAddr, size_t len) {
    switch (operation_mode) {
        case ANALYSER_OPERATION_MODE_ANALYSIS:
            //printf("!");
            if (analysis_enabled) {
                analyser_result_t analyserResult = analyser_analyse(*ppuDataBufAddr, len);
                switch (analyserResult) {
                    case ANALYSER_RESULT_CALM:
                        break;
                    case ANALYSER_RESULT_TRIGGERED:
                        x_analyser_perform_action(width, height, format, ppuDataBufAddr, len);
                        break;
                    default:
                        ESP_LOGE(TAG, "Unknown analyser result!");
                        break;
                }
            }
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

