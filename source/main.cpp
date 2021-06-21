/* Edge Impulse ingestion SDK
 * Copyright (c) 2020 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "mbed.h"
#include "mbed_mem_trace.h"
#include "LittleFileSystem.h"
#include "model-parameters/model_metadata.h"
#include "ei_run_classifier.h"
#include "at_cmd_repl_mbed.h"
#include "mbed_fs_commands.h"
#include "model_metadata.h"
#include "ei_microphone.h"
#include "ei_microphone_inference.h"

// set up two slices on block device, one for the file system, and one for temporary files
static BlockDevice *full_bd = BlockDevice::get_default_instance();
static SlicingBlockDevice fs_bd(full_bd, 0x0, 6 * 1024 * 1024);
SlicingBlockDevice temp_bd(full_bd, 6 * 1024 * 1024, 8 * 1024 * 1024);

static LittleFileSystem fs("fs", &fs_bd, 256, 512, 512);
EventQueue main_application_queue;
DigitalOut led(LED1);

const uint8_t PREDICTION_OUTPUT_CONFIG_SHORT = 0x00; //short output
const uint8_t PREDICTION_OUTPUT_CONFIG_LONG = 0x01; //long output
const uint8_t PREDICTION_OUTPUT_CONFIG_SCI = 0xFF; //speech command interface output
static uint8_t _prediction_output_config=PREDICTION_OUTPUT_CONFIG_SCI;
static bool _prediction_output_good_only=true;

// TODO:redirect this to other serial
#ifndef AT_OUTPUT
#define AT_OUTPUT printf
#endif //AT_OUTPUT

//#ifndef AT_DEBUG_OUTPUT
//#define AT_DEBUG_OUTPUT printf
//#endif //AT_DEBUG_OUTPUT
static bool AT_DEBUG_OUTPUT_ENABLE = false;
void AT_DEBUG_OUTPUT(const char *format, ...){
    if(true==AT_DEBUG_OUTPUT_ENABLE){
        va_list myargs;
        va_start(myargs, format);
        printf("[DEBUG]");
        vprintf(format, myargs);
        va_end(myargs);
    }
}

void set_debug_output_en(char *arg){
    uint8_t good_only = atoi(arg);
    switch (good_only) {
        case 0x01:
            AT_DEBUG_OUTPUT_ENABLE = true;
            AT_OUTPUT("OK\r\n");
            break;
        case 0x00:
            AT_DEBUG_OUTPUT_ENABLE = false;
            AT_OUTPUT("OK\r\n");
            break;
        default:
            AT_OUTPUT("ERROR\r\n");
            break;
    }
}

void at_get_prediction_output(){
    switch (_prediction_output_config) {
        case PREDICTION_OUTPUT_CONFIG_SHORT:
            AT_OUTPUT("+CONFIGPREDICTIONOUTPUT=SHORT\r\n");
            AT_OUTPUT("OK\r\n");
            break;
        case PREDICTION_OUTPUT_CONFIG_LONG:
            AT_OUTPUT("+CONFIGPREDICTIONOUTPUT=LONG\r\n");
            AT_OUTPUT("OK\r\n");
            break;
        case PREDICTION_OUTPUT_CONFIG_SCI:
            AT_OUTPUT("+CONFIGPREDICTIONOUTPUT=SCI\r\n");
            AT_OUTPUT("OK\r\n");
            break;
        default:
            AT_OUTPUT("ERROR\r\n");
            break;
    }
}

void at_set_prediction_output(char* arg){
    uint8_t temp=atoi(arg);
    switch (temp) {
        case PREDICTION_OUTPUT_CONFIG_SHORT: //SHORT output
            _prediction_output_config = PREDICTION_OUTPUT_CONFIG_SHORT;
            AT_OUTPUT("OK\r\n");
            break;
        case PREDICTION_OUTPUT_CONFIG_LONG: //LONG output
            _prediction_output_config = PREDICTION_OUTPUT_CONFIG_LONG;
            AT_OUTPUT("OK\r\n");
            break;
        default: //SCI output
            _prediction_output_config = PREDICTION_OUTPUT_CONFIG_SCI;
            AT_OUTPUT("OK\r\n");
            break;
    }
}

void at_filter_prediction(char *arg){
    uint8_t good_only = atoi(arg);
    switch (good_only) {
        case 0x01:
            _prediction_output_good_only = true;
            AT_OUTPUT("OK\r\n");
            break;
        case 0x00:
            _prediction_output_good_only = false;
            AT_OUTPUT("OK\r\n");
            break;
        default:
            AT_OUTPUT("ERROR\r\n");
            break;
    }
}

static unsigned char repl_stack[4 * 1024];
static AtCmdRepl repl(&main_application_queue, sizeof(repl_stack), repl_stack);

static WiFiInterface *network = NULL;
static bool network_connected = false;

// this lists all the sensors that this device has (initialized later on, when we know whether
// sensors are present on the board)
static ei_sensor_t sensor_list[2] = { 0 };

static bool microphone_present;

//extern char* ei_classifier_inferencing_categories[];

void print_memory_info() {
    // Grab the heap statistics
    mbed_stats_heap_t heap_stats;
    mbed_stats_heap_get(&heap_stats);
    printf("Heap size: %lu / %lu bytes (max: %lu)\r\n", heap_stats.current_size, heap_stats.reserved_size, heap_stats.max_size);
}

#if defined(EI_CLASSIFIER_SENSOR) && EI_CLASSIFIER_SENSOR == EI_CLASSIFIER_SENSOR_MICROPHONE

typedef struct {
    float probability;
    size_t class_id;
    bool is_ok;
} prediction_t;
prediction_t my_prediction;

#define CLASS_NOISE_INDEX 0
#define CLASS_UNKNOWN_INDEX 1
#define MEANINGFUL_CLASS_INDEX 1
static float PREDICT_THRESHOLD=0.8;

void at_get_predict_theadshold(){
    AT_DEBUG_OUTPUT("+PTHRES=%.2f\r\n", PREDICT_THRESHOLD);
    return;
}

void at_set_predict_theadshold(char* arg){
    float threshold = atof(arg);
    if(threshold<1.0){
        PREDICT_THRESHOLD = threshold;
        AT_OUTPUT("+PTHRES=%.2f\r\n", PREDICT_THRESHOLD);
        AT_OUTPUT("OK\r\n");
    }
    else{
        AT_DEBUG_OUTPUT("ERROR\r\n");
    }
    return;
}

void get_supported_class(){
    AT_OUTPUT("+CLASSLIST=");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        AT_OUTPUT("%s,", ei_classifier_inferencing_categories[ix]); //this is bad
    }
    AT_OUTPUT("\r\n");
}

int get_prediction(prediction_t* prediction, ei_impulse_result_t* result){
    prediction->probability = 0.0;
    prediction->is_ok = false;
    for (size_t ix = MEANINGFUL_CLASS_INDEX; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        // get max_probability
        if(result->classification[ix].value > prediction->probability) {
            prediction->probability = result->classification[ix].value;
            prediction->class_id = ix;
        }
    }
    if(result->classification[prediction->class_id].value<PREDICT_THRESHOLD){
        prediction->is_ok = false;
    }
    else {
        prediction->is_ok = true;
    }
    return 0;
}

int send_prediction(ei_impulse_result_t* result){
    // print the predictions
    switch(_prediction_output_config){
        case PREDICTION_OUTPUT_CONFIG_SHORT:
            get_prediction(&my_prediction, result);
            AT_DEBUG_OUTPUT("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                result->timing.dsp, result->timing.classification, result->timing.anomaly);
            AT_OUTPUT("    %s: %.5f\n", result->classification[my_prediction.class_id].label, result->classification[my_prediction.class_id].value);
            AT_OUTPUT("    %s prediction\n", my_prediction.is_ok?"GOOD":"BAD");
            break;
        case PREDICTION_OUTPUT_CONFIG_LONG:
            AT_DEBUG_OUTPUT("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                result->timing.dsp, result->timing.classification, result->timing.anomaly);
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                AT_OUTPUT("    %s: %.5f\n", result->classification[ix].label, result->classification[ix].value);
            }
            break;
        default: //PREDICTION_OUTPUT_CONFIG_SCI
            get_prediction(&my_prediction, result);
            if (false==_prediction_output_good_only) {
                AT_OUTPUT("+UPCLA=%s,%.5f,%s\r\n", result->classification[my_prediction.class_id].label,
                        result->classification[my_prediction.class_id].value,
                        my_prediction.is_ok?"GOOD":"BAD"
                        );
            }
            else{
                if(my_prediction.is_ok){
                    AT_OUTPUT("+UPCLA=%s,%.5f\r\n", result->classification[my_prediction.class_id].label,
                            result->classification[my_prediction.class_id].value
                            );
                }
            }
            break;
    }
    return 0;
}

void print_inference_summary(){
    // summary of inferencing settings (from model_metadata.h)
    AT_DEBUG_OUTPUT("Inferencing settings:\n");
    AT_DEBUG_OUTPUT("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    AT_DEBUG_OUTPUT("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    AT_DEBUG_OUTPUT("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    AT_DEBUG_OUTPUT("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
}

void run_nn_single(){
    _prediction_output_good_only = false; //disable prediction filter

    AT_DEBUG_OUTPUT("Recording\n");

    bool m = ei_microphone_record(EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16, 20, false);
    if (!m) {
        AT_DEBUG_OUTPUT("ERR: Failed to record audio...\n");
        return;
    }

    AT_DEBUG_OUTPUT("Recording OK\n");

    AT_DEBUG_OUTPUT("Starting inferencing \n");
    signal_t signal = ei_microphone_get_signal();
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, false);
    if (r != EI_IMPULSE_OK) {
        AT_DEBUG_OUTPUT("ERR: Failed to run classifier (%d)\n", r);
        return;
    }

    // print the predictions
    send_prediction(&result);

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    AT_DEBUG_OUTPUT("    anomaly score: %.3f\n", result.anomaly);
#endif

    AT_OUTPUT("OK\n");
}

void run_nn(bool debug) {
    print_inference_summary();

    AT_DEBUG_OUTPUT("Starting inferencing, press 'b' to break\n");

    while (1) {
        AT_DEBUG_OUTPUT("Starting inferencing in 2 seconds...\n");

        // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
        if (ei_sleep(2000) != EI_IMPULSE_OK) {
            break;
        }

        AT_DEBUG_OUTPUT("Recording\n");

        bool m = ei_microphone_record(EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16, 20, false);
        if (!m) {
            AT_DEBUG_OUTPUT("ERR: Failed to record audio...\n");
            break;
        }

        AT_DEBUG_OUTPUT("Recording OK\n");

        signal_t signal = ei_microphone_get_signal();
        ei_impulse_result_t result = { 0 };

        EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug);
        if (r != EI_IMPULSE_OK) {
            AT_DEBUG_OUTPUT("ERR: Failed to run classifier (%d)\n", r);
            break;
        }

        // print the predictions
        send_prediction(&result);

#if EI_CLASSIFIER_HAS_ANOMALY == 1
        AT_DEBUG_OUTPUT("    anomaly score: %.3f\n", result.anomaly);
#endif
    }
}

void run_nn_continuous(bool debug) {

    int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);

    print_inference_summary();

    AT_DEBUG_OUTPUT("Starting inferencing, press 'b' to break\n");

    run_classifier_init();
    ei_microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE);

    while (1) {

        bool m = ei_microphone_inference_record();
        if (!m) {
            AT_DEBUG_OUTPUT("ERR: Failed to record audio...\n");
            break;
        }

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
        signal.get_data = &ei_microphone_audio_signal_get_data;
        ei_impulse_result_t result = { 0 };

        EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug);
        if (r != EI_IMPULSE_OK) {
            AT_DEBUG_OUTPUT("ERR: Failed to run classifier (%d)\n", r);
            break;
        }

        if(++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW >> 1)) {
            // print the predictions
            send_prediction(&result);
            #if EI_CLASSIFIER_HAS_ANOMALY == 1
            AT_DEBUG_OUTPUT("    anomaly score: %.3f\n", result.anomaly);
            #endif

            print_results = 0;
        }
    }

    ei_microphone_inference_end();
}

void run_nn_continuous_normal() {
    run_nn_continuous(false);
}

void run_nn_continuous_debug() {
    run_nn_continuous(true);
}

#else

#error "EI_CLASSIFIER_SENSOR not configured, cannot configure `run_nn`"

#endif // EI_CLASSIFIER_SENSOR

void run_nn_normal() {
    run_nn(false);
}

void run_nn_debug() {
    run_nn(true);
}

void fill_memory() {
    size_t size = 8 * 1024;
    size_t allocated = 0;
    while (1) {
        void *ptr = malloc(size);
        if (!ptr) {
            if (size == 1) break;
            size /= 2;
        }
        else {
            allocated += size;
        }
    }
    printf("Allocated: %u bytes\n", allocated);
}

void prvAtCmdInit(){
    ei_at_register_generic_cmds();
    #if defined(EI_CLASSIFIER_SENSOR) && EI_CLASSIFIER_SENSOR == EI_CLASSIFIER_SENSOR_MICROPHONE
    //ei_at_cmd_register("FILLMEMORY", "Try and fill the full RAM, to report free heap stats", fill_memory);
    ei_at_cmd_register("RUNSINGLE", "Run a single prediction", run_nn_single);
    //ei_at_cmd_register("RUNIMPULSE", "Run the impulse", run_nn_normal);
    //ei_at_cmd_register("RUNIMPULSEDEBUG", "Run the impulse with debug messages", run_nn_debug);
    ei_at_cmd_register("RUNCONT", "Run the impulse continuously", run_nn_continuous_normal);
    //ei_at_cmd_register("RUNIMPULSECONT", "Run the impulse continuously", run_nn_continuous_normal);
    //ei_at_cmd_register("RUNIMPULSECONTDEBUG", "Run the impulse continuously with debug messages", run_nn_continuous_debug);
    //TODO: rename this commands
    ei_at_cmd_register("CONFPOUTPUT=", "set prediction output format", at_set_prediction_output);
    ei_at_cmd_register("CONFPOUTPUT?", "get prediction output format", at_get_prediction_output);
    ei_at_cmd_register("PTHRES=", "set prediction probability threshold", at_set_predict_theadshold);
    ei_at_cmd_register("PTHRES?", "get prediction probability threshold", at_get_predict_theadshold);
    ei_at_cmd_register("PFILTER=", "config get only good prediction (affect only SCI FORMAT)", at_filter_prediction);
    ei_at_cmd_register("DOUTPUTEN=", "enable debug output", set_debug_output_en);
    ei_at_cmd_register("CLASSLIST", "enable debug output", get_supported_class);
    #endif
}

int main() {
    // mbed_mem_trace_set_callback(mbed_mem_trace_default_callback);
    AT_DEBUG_OUTPUT("Compiled on %s at %s\n", __DATE__, __TIME__);

    int err = fs.mount(&fs_bd);
    AT_DEBUG_OUTPUT("Mounting file system: %s\n", (err ? "Fail :(" : "OK"));
    if (err) {
        // Reformat if we can't mount the filesystem
        // this should only happen on the first boot
        AT_DEBUG_OUTPUT("No filesystem found, formatting... ");
        fflush(stdout);
        err = fs.reformat(&fs_bd);
        AT_DEBUG_OUTPUT("%s\n", (err ? "Fail :(" : "OK"));
        if (err) {
            error("error: %s (%d)\n", strerror(-err), err);
        }
        err = fs.mount(&fs_bd);
        if (err) {
            AT_DEBUG_OUTPUT("Failed to mount file system (%d)\n", err);
            return 1;
        }
    }

    err = temp_bd.init();
    if (err != 0) {
        AT_DEBUG_OUTPUT("Failed to initialize temp blockdevice\n");
        return 1;
    }

    // Sensor init
    microphone_present = ei_microphone_init();

    // intialize configuration
    prvAtCmdInit();

    AT_DEBUG_OUTPUT("Type AT+HELP to see a list of commands.\n");

    AT_DEBUG_OUTPUT("------------------------\n");
    AT_DEBUG_OUTPUT("SPEECH COMMAND INTERFACE\n");

     print_memory_info();

     //running communication interface
    repl.start_repl();

    main_application_queue.dispatch_forever();
}
