#include <stdint.h>
#include <stdlib.h>

typedef struct {
    /* JFIF data buffer */
    uint8_t* puData;
    /* Length of the JFIF data buffer */
    size_t len;
    size_t imageWidth;
    size_t imageHeight;
} tl_infer_agent_input_t;

typedef enum {
    TL_INFER_AGENT_OUTPUT_NO_ACTION = 0U,
    TL_INFER_AGENT_OUTPUT_TRIGGER_ACTION = 1U
} tl_infer_agent_output_t;

#ifdef __cplusplus
extern "C" {
#endif
void tl_infer_agent_init();

void tl_infer_agent_feed_input(tl_infer_agent_input_t* pInput);
void tl_infer_agent_perform_inference();
tl_infer_agent_output_t tl_infer_agent_get_output();

#ifdef __cplusplus
}
#endif

