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

#ifndef _EI_MICROPHONE_H_
#define _EI_MICROPHONE_H_

#include "stm32l475e_iot01_audio.h"
#include "ei_microphone_inference.h"

/**
 * todo: we're using raw block device here because it's much faster
 *       however, we don't do wear-leveling at this point because of this
 *       we need to implement this.
 */

#define BD_ERASE_SIZE                   4096

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t mic_params;

// buffer, needs to be aligned to temp flash erase size
#define AUDIO_BUFFER_NB_SAMPLES         2048

// we'll use only half the buffer at the same time, because we have flash latency
// of up to 70 ms.
static int16_t AUDIO_BUFFER[AUDIO_BUFFER_NB_SAMPLES];
static size_t AUDIO_BUFFER_IX = 0;

#define AUDIO_THREAD_STACK_SIZE             4096
static uint8_t AUDIO_THREAD_STACK[AUDIO_THREAD_STACK_SIZE];

// skip the first 2000 ms.
static bool is_recording = false;
static uint32_t audio_event_count = 0;
static uint32_t max_audio_event_count = 0;
static size_t mic_header_length = 0;

extern SlicingBlockDevice temp_bd;
static EventQueue mic_queue;
static size_t TEMP_FILE_IX = 0;

extern void print_memory_info();
extern EventQueue main_application_queue;
extern DigitalOut led;

static void ei_microphone_blink() {
    led = !led;
}

enum AUDIO_BUFFER_EVENT {
    HALF = 0,
    FULL = 1,
    FINALIZE = 2,
    NONE = 3
};

static AUDIO_BUFFER_EVENT last_audio_buffer_event = NONE;

static void audio_buffer_callback(AUDIO_BUFFER_EVENT event) {
    int16_t *start = AUDIO_BUFFER + ((AUDIO_BUFFER_NB_SAMPLES / 2) * event);
    size_t buffer_size = (AUDIO_BUFFER_NB_SAMPLES / 2) * sizeof(int16_t);

    if (event == FINALIZE) {
        if (last_audio_buffer_event == HALF) {
            start = AUDIO_BUFFER + ((AUDIO_BUFFER_NB_SAMPLES / 2) * FULL);
            buffer_size = (AUDIO_BUFFER_IX - (AUDIO_BUFFER_NB_SAMPLES / 2)) * sizeof(int16_t);
        }
        else {
            start = AUDIO_BUFFER + ((AUDIO_BUFFER_NB_SAMPLES / 2) * HALF);
            buffer_size = AUDIO_BUFFER_IX * sizeof(int16_t);
        }
    }

    int ret = temp_bd.program(start, TEMP_FILE_IX, buffer_size);
    TEMP_FILE_IX += buffer_size;

    last_audio_buffer_event = event;

    if (ret != 0) {
        printf("ERR: audio_buffer_callback %d write failed (%d)\n", event, ret);
    }
}

static void finish_and_upload(char *filename, uint32_t sample_length_ms) {
}

/**
 * @brief      Called from the BSP audio callback function
 *
 * @param[in]  halfOrCompleteTransfer  Signal half or complete to indicate offset in sampleBuffer
 */
static void get_audio_from_dma_buffer(bool halfOrCompleteTransfer)
{
    if (!is_recording) return;
    if (audio_event_count >= max_audio_event_count) return;
    audio_event_count++;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2;           /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    if ((AUDIO_BUFFER_IX + nb_samples) > AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }

    /* Copy second half of PCM_Buffer from Microphones onto Fill_Buffer */
    if (halfOrCompleteTransfer) {
        memcpy(AUDIO_BUFFER + AUDIO_BUFFER_IX, PCM_Buffer + nb_samples, buffer_size);
    } else {
        memcpy(AUDIO_BUFFER + AUDIO_BUFFER_IX, PCM_Buffer, buffer_size);
    }
    AUDIO_BUFFER_IX += nb_samples;

    if (AUDIO_BUFFER_IX == (AUDIO_BUFFER_NB_SAMPLES / 2)) {
        // half full
        mic_queue.call(&audio_buffer_callback, HALF);
    } else if (AUDIO_BUFFER_IX == AUDIO_BUFFER_NB_SAMPLES) {
        // completely full
        AUDIO_BUFFER_IX = 0;
        mic_queue.call(&audio_buffer_callback, FULL);
    }
}

/**
  * @brief  Manages the BSP audio in error event.
  * @param  Instance Audio in instance.
  * @retval None.
  */
void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance) {
    printf("BSP_AUDIO_IN_Error_CallBack\n");
}

bool ei_microphone_init() {
    mic_params.BitsPerSample = 16;
    mic_params.ChannelsNbr = AUDIO_CHANNELS;
    mic_params.Device = AUDIO_IN_DIGITAL_MIC1;
    mic_params.SampleRate = AUDIO_SAMPLING_FREQUENCY;
    mic_params.Volume = 32;

    int32_t ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &mic_params);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Init (%ld)\r\n", ret);
        return false;
    }
    return true;
}

bool ei_microphone_record(uint32_t sample_length_ms, uint32_t start_delay_ms, bool print_start_messages) {
    if (is_recording) return false;

    Thread queue_thread(osPriorityHigh, AUDIO_THREAD_STACK_SIZE, AUDIO_THREAD_STACK, "audio-thread-stack");
    queue_thread.start(callback(&mic_queue, &EventQueue::dispatch_forever));

    int32_t ret;
    uint32_t state;
    ret = BSP_AUDIO_IN_GetState(AUDIO_INSTANCE, &state);
    if (ret != BSP_ERROR_NONE) {
        printf("Cannot start recording: Error getting audio state (%ld)\n", ret);
        return false;
    }
    if (state == AUDIO_IN_STATE_RECORDING) {
        printf("Cannot start recording: Already recording\n");
        return false;
    }

    AUDIO_BUFFER_IX = 0;
    is_recording = false;
    audio_event_count = 0;
    max_audio_event_count = sample_length_ms; // each event is 1ms., very convenient
    TEMP_FILE_IX = 0;

    ei_microphone_inference_set_bsp_callback(&get_audio_from_dma_buffer);

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printf("Error failed to start recording (%ld)\n", ret);
        return false;
    }

    if (print_start_messages) {
        printf("Starting in %lu ms... (or until all flash was erased)\n", start_delay_ms);
    }

    // clear out flash memory
    size_t blocks = static_cast<size_t>(ceil(
        (static_cast<float>(sample_length_ms) * static_cast<float>(AUDIO_SAMPLING_FREQUENCY / 1000) * 2.0f) /
        BD_ERASE_SIZE)) + 1;
    // printf("Erasing %lu blocks\n", blocks);

    Timer erase_timer;
    erase_timer.start();

    for (size_t ix = 0; ix < blocks; ix++) {
        int r = temp_bd.erase(ix * BD_ERASE_SIZE, BD_ERASE_SIZE);
        if (r != 0) {
            printf("Failed to erase block device #2 (%d)\n", r);
            return false;
        }
    }

    // Gonna write the header to the header block
    // the issue here is that we bound the context already to FILE
    // which we don't have here... hmm
    {
        mic_header_length = 0;


        TEMP_FILE_IX = mic_header_length;
    }

    erase_timer.stop();
    // printf("Erase timer took %d ms.\n", erase_timer.read_ms());

    if (erase_timer.read_ms() > static_cast<int>(start_delay_ms)) {
        start_delay_ms = 0;
    }
    else {
        start_delay_ms = start_delay_ms - erase_timer.read_ms();
    }

    ThisThread::sleep_for(start_delay_ms);


    led = 1;

    is_recording = true;

    ThisThread::sleep_for(sample_length_ms);

    // we're not perfectly aligned  with the audio interrupts, so give some extra time to ensure
    // we always have at least the sample_length_ms number of frames
    int32_t extra_time_left = 1000;
    while (audio_event_count < max_audio_event_count) {
        ThisThread::sleep_for(10);
        extra_time_left -= 10;
        if (extra_time_left <= 0) {
            break;
        }
    }

    led = 0;

    is_recording = false;

    ret = BSP_AUDIO_IN_Stop(AUDIO_INSTANCE);
    if (ret != BSP_ERROR_NONE) {
        printf("Error failed to stop recording (%ld)\n", ret);
        return false;
    }

    // give the mic queue some time to process last events
    // mbed eventqueue should really have a way of knowing when the queue is empty
    ThisThread::sleep_for(100);

    audio_buffer_callback(FINALIZE);

    // too much data? cut it
    if (TEMP_FILE_IX > mic_header_length + (sample_length_ms * (AUDIO_SAMPLING_FREQUENCY / 1000) * 2)) {
        TEMP_FILE_IX = mic_header_length + (sample_length_ms * (AUDIO_SAMPLING_FREQUENCY / 1000) * 2);
    }

    return true;
}

/**
 * Sample raw data
 */
bool ei_microphone_sample_start() {
}

/**
 * Get raw audio signal data
 */
static int raw_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    offset += mic_header_length;

    EIDSP_i16 *temp = (EIDSP_i16*)calloc(length, sizeof(EIDSP_i16));
    if (!temp) {
        printf("raw_audio_signal_get_data - malloc failed\n");
        return EIDSP_OUT_OF_MEM;
    }

    int r = temp_bd.read(temp, offset * sizeof(EIDSP_i16), length * sizeof(EIDSP_i16));
    if (r != 0) {
        printf("raw_audio_signal_get_data - BD read failed (%d)\n", r);
        free(temp);
        return r;
    }

    r = numpy::int16_to_float(temp, out_ptr, length);
    free(temp);

    return r;
}

/**
 * Get signal from the just recorded data (not yet processed)
 */
signal_t ei_microphone_get_signal() {
    signal_t signal;
    signal.total_length = (TEMP_FILE_IX - mic_header_length) / sizeof(int16_t);
    signal.get_data = &raw_audio_signal_get_data;
    return signal;
}

#endif // _EI_MICROPHONE_H_
