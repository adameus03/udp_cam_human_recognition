#include <stdint.h>
#include <stddef.h>

typedef enum {
    ANALYSER_OPERATION_MODE_ANALYSIS,
    ANALYSER_OPERATION_MODE_UNCONDITIONAL_STREAM
} analyser_operation_mode_t;

typedef enum {
    ANALYSER_RESULT_CALM,
    ANALYSER_RESULT_TRIGGERED
} analyser_result_t;

void analyser_init();
void analyser_run(uint32_t usStackDepth);
void analyser_set_operation_mode(analyser_operation_mode_t mode);
void analyser_shutdown();
void analyser_wakeup();

analyser_result_t analyser_analyse(uint8_t* puDataBuf, size_t len);

extern void x_analyser_perform_action(int width, int height, int format, uint8_t** ppuDataBufAddr, size_t len);