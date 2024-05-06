#include <stdint.h>
#include <stddef.h>

typedef enum {
    ANALYSER_OPERATION_MODE_ANALYSIS,
    ANALYSER_OPERATION_MODE_UNCONDITIONAL_STREAM
} analyser_operation_mode_t;

void analyser_init();
void analyser_run(uint32_t usStackDepth);
void analyser_set_operation_mode(analyser_operation_mode_t mode);

extern void x_analyser_perform_action(int width, int height, int format, uint8_t** ppuDataBufAddr, size_t len);