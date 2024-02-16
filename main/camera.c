#include "camera.h"
#include "esp_log.h"

//#define CONFIG_BOARD_ESP32CAM_AITHINKER 1
// AiThinker ESP32Cam PIN Map
//#if CONFIG_BOARD_ESP32CAM_AITHINKER
#define CAM_PIN_PWDN  32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK   0
#define CAM_PIN_SIOD  26
#define CAM_PIN_SIOC  27

#define CAM_PIN_D7    35 // Y9
#define CAM_PIN_D6    34 // Y8
#define CAM_PIN_D5    39 // Y7
#define CAM_PIN_D4    36 // Y6
#define CAM_PIN_D3    21 // Y5
#define CAM_PIN_D2    19 // Y4
#define CAM_PIN_D1    18 // Y3
#define CAM_PIN_D0     5 // Y2
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF  23
#define CAM_PIN_PCLK  22
//#endif

static const char * TAG = "camera";

static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    //.frame_size = FRAMESIZE_UXGA,//QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
    .frame_size = FRAMESIZE_SXGA,

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1, //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    //.grab_mode = CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled
    .grab_mode = CAMERA_GRAB_LATEST
};

esp_err_t camera_init(int frame_size, int jpeg_quality){
    esp_log_level_set(TAG, ESP_LOG_DEBUG); // Set log level to debug
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    //change the frame size
    sensor_t * s = esp_camera_sensor_get();
    s->set_framesize(s, (framesize_t)frame_size);
    //change the jpeg quality
    s->set_quality(s, jpeg_quality);
    return ESP_OK;
}

esp_err_t camera_capture(){
    //acquire a frame
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Got frame of size %zu", fb->len);

    //Process the frame using the user's function
    x_camera_process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);
  
    //return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    return ESP_OK;
}

esp_err_t camera_set_framesize(int frame_size){
    sensor_t * s = esp_camera_sensor_get();
    return s->set_framesize(s, (framesize_t)frame_size);
}

esp_err_t camera_set_quality(int jpeg_quality){
    sensor_t * s = esp_camera_sensor_get();
    return s->set_quality(s, jpeg_quality);
}