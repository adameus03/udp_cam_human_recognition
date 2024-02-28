#include "esp_camera.h"

/**
 * @brief Initialize the camera
 * @param frame_size The frame size to use
 * @param jpeg_quality The JPEG quality to use
 * @return ESP_OK on success
 * @note This function should be called before any other camera functions
 * @note This function should be called only once
 * @note The frame size and JPEG quality can be changed later using the camera_set_framesize and camera_set_quality functions
*/
esp_err_t camera_init(int frame_size, int jpeg_quality);

/**
 * @brief Capture an image
 * @return ESP_OK on success
 * @note This function should be called after the camera_init function
 * @note This function should be called in a loop to capture images continuously
 * @note The captured image is processed by the process_image function
*/
esp_err_t camera_capture();

/**
 * @brief Set the frame size
 * @param frame_size The frame size to use
 * @return ESP_OK on success
*/
esp_err_t camera_set_framesize(int frame_size);

/**
 * @brief Set the JPEG quality
 * @param jpeg_quality The JPEG quality to use
 * @return ESP_OK on success
*/
esp_err_t camera_set_quality(int jpeg_quality);
/**
 * @brief Process a captured image
 * @note This function is called from the camera capture function
 * @note This function should be implemented by the user
*/
extern void x_camera_process_image(int width, int height, int format, uint8_t** ppuDataBufAddr, size_t len);