#include <stdint.h>
#include <stddef.h>
void analyser_init();
void analyser_run();
extern void x_analyser_perform_action(int width, int height, int format, uint8_t * data, size_t len);