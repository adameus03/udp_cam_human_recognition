#include <stdint.h>
#include <stddef.h>
void analyser_init();
void analyser_run(uint32_t usStackDepth);
extern void x_analyser_perform_action(int width, int height, int format, uint8_t** ppuDataBufAddr, size_t len);