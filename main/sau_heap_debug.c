#include "stddef.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "heap_debug"; 

/**
 * Heap debug function
*/
void sau_heap_debug_info() {
    size_t spiram_heap_free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t spiram_largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    size_t dma_heap_free_size = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t dma_largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
    size_t default_heap_free_size = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t default_largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    ESP_LOGI(TAG, " ");
    ESP_LOGI(TAG, "spiram_heap_free_size = %u", spiram_heap_free_size);
    ESP_LOGI(TAG, "spiram_largest_free_block = %u", spiram_largest_free_block);
    ESP_LOGI(TAG, "dma_heap_free_size = %u", dma_heap_free_size);
    ESP_LOGI(TAG, "dma_largest_free_block = %u", dma_largest_free_block);
    ESP_LOGI(TAG, "default_heap_free_size = %u", default_heap_free_size);
    ESP_LOGI(TAG, "default_largest_free_block = %u", default_largest_free_block);
    ESP_LOGI(TAG, " ");
}