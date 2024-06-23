#include "tl_infer_agent.h"
#include "tl_model.h"

#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "img_converters.h"

#include "esp_log.h"

#define TF_INFER_AGENT_NUM_COLS 224
#define TF_INFER_AGENT_NUM_ROWS 224
#define TF_INFER_AGENT_NUM_CHANNELS 3
#define TF_INFER_AGENT_CATEGORY_COUNT 2
#define TF_INFER_AGENT_NOTIFY_INDEX 1
#define TF_INFER_AGENT_NOP_INDEX 0

static const char* TAG = "tl_infer_agent"; 

namespace {
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    int inference_count = 0;

    constexpr int kTensorArenaSize = 2000;
    uint8_t tensor_arena[kTensorArenaSize];
}

struct __tl_infer_agent_model_settings {
    int kNumCols;
    int kNumRows;
    int kNumChannels;
    int kMaxImageSize;
    int kCategoryCount;
    int kNotifyIndex;
    int kNopIndex;
} model_settings;

void tl_infer_agent_init() {

    model_settings.kNumCols = TF_INFER_AGENT_NUM_COLS;
    model_settings.kNumRows = TF_INFER_AGENT_NUM_ROWS;
    model_settings.kNumChannels = TF_INFER_AGENT_NUM_CHANNELS;
    model_settings.kMaxImageSize = TF_INFER_AGENT_NUM_COLS * TF_INFER_AGENT_NUM_ROWS * TF_INFER_AGENT_NUM_CHANNELS;
    model_settings.kCategoryCount = TF_INFER_AGENT_CATEGORY_COUNT;
    model_settings.kNotifyIndex = TF_INFER_AGENT_NOTIFY_INDEX;
    model_settings.kNopIndex = TF_INFER_AGENT_NOP_INDEX;


    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    model = tflite::GetModel(tl_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("Model provided is schema version %d not equal to supported "
                    "version %d.", model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    // Pull in only the operation implementations we need.
    static tflite::MicroMutableOpResolver<5> resolver;
    /*if (resolver.AddFullyConnected() != kTfLiteOk) {
        return;
    }*/
    
    // [TODO] @verify
    if (kTfLiteOk != resolver.AddAveragePool2D()) { ESP_LOGE(TAG, "AddAveragePool2D failed"); assert(0); }
    if (kTfLiteOk != resolver.AddConv2D()) { ESP_LOGE(TAG, "AddConv2D failed"); assert(0); }
    if (kTfLiteOk != resolver.AddDepthwiseConv2D()) { ESP_LOGE(TAG, "AddDepthwiseConv2D failed"); assert(0); }
    if (kTfLiteOk != resolver.AddReshape()) { ESP_LOGE(TAG, "AddReshape failed"); assert(0); }
    if (kTfLiteOk != resolver.AddSoftmax()) { ESP_LOGE(TAG, "AddSoftmax failed"); assert(0); }

    // Build an interpreter to run the model with.
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        MicroPrintf("AllocateTensors() failed");
        return;
    }

    // Obtain pointers to the model's input and output tensors.
    input = interpreter->input(0);
    output = interpreter->output(0);

    // Keep track of how many inferences we have performed.
    inference_count = 0;
}

void __tf_infer_agent_prepare_input(uint8_t* pRgb565buf, size_t width, size_t height, int8_t* pInput_out) {
    // [TODO] Implement
    ESP_LOGE(TAG, "Not implemented (__tf_infer_agent_prepare_input)");
    assert(0);
}

/**
 * @param pInput Pointer to the input JFIF image data
*/
void tl_infer_agent_feed_input_jfif(tl_infer_agent_input_t* pInput) {
    ESP_LOGE(TAG, "Not implemented (tl_infer_agent_feed_input)");
    assert(0);

    //Perform conversion of JFIF image data to RGB565 format
    // uint8_t* rgb565buf = (uint8_t*)malloc(model_settings.kMaxImageSize);
    uint8_t* rgb565buf = (uint8_t*)malloc(pInput->imageWidth * pInput->imageHeight * 3);
    jpg2rgb565(pInput->puData, pInput->len, rgb565buf, JPG_SCALE_NONE);

    // Convert image size to TF_INFER_AGENT_NUM_ROWS x TF_INFER_AGENT_NUM_COLS and feed it to the model
    

}

void tl_infer_agent_perform_inference() {
    // Run the model on this input and make sure it succeeds.
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        ESP_LOGE(TAG, "interpreter->Invoke failed");
        assert(0);
    }
}

tl_infer_agent_output_t tl_infer_agent_get_output() {
    int8_t _notify_score = output->data.uint8[model_settings.kNotifyIndex];
    int8_t _nop_score = output->data.uint8[model_settings.kNopIndex];

    float notify_score = (_notify_score - output->params.zero_point) * output->params.scale;
    float nop_score = (_nop_score - output->params.zero_point) * output->params.scale;

    // [TODO] @verify
    if (notify_score > nop_score) {
        return TL_INFER_AGENT_OUTPUT_TRIGGER_ACTION;
    } else {
        return TL_INFER_AGENT_OUTPUT_NO_ACTION;
    }
}