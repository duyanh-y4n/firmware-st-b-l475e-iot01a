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

#ifndef _EDGE_IMPULSE_AT_COMMANDS_CONFIG_H_
#define _EDGE_IMPULSE_AT_COMMANDS_CONFIG_H_

#include "at_cmd_interface.h"
#include "at_base64.h"
#include "ei_config.h"

#define EDGE_IMPULSE_AT_COMMAND_VERSION        "1.3.0"

static void at_clear_config() {
    printf("Clearing config and restarting system...\n");
    ei_config_clear();
    NVIC_SystemReset();
}

static void at_device_info() {
    uint8_t id_buffer[32] = { 0 };
    size_t id_size;
    int r = ei_config_get_device_id(id_buffer, &id_size);
    if (r == EI_CONFIG_OK) {
        id_buffer[id_size] = 0;
        printf("ID:         %s\n", id_buffer);
    }
    if (ei_config_get_context()->get_device_type == NULL) {
        return;
    }
    r = ei_config_get_context()->get_device_type(id_buffer, &id_size);
    if (r == EI_CONFIG_OK) {
        id_buffer[id_size] = 0;
        printf("Type:       %s\n", id_buffer);
    }
    printf("AT Version: %s\n", EDGE_IMPULSE_AT_COMMAND_VERSION);
}

static void at_list_sensors() {
    if (ei_config_get_context()->get_sensor_list == NULL) {
        return;
    }

    const ei_sensor_t *list;
    size_t list_size;

    int r = ei_config_get_context()->get_sensor_list(&list, &list_size);
    if (r != 0) {
        printf("Failed to get sensor list (%d)\n", r);
        return;
    }

    for (size_t ix = 0; ix < list_size; ix++) {
        printf("Name: %s, Max sample length: %hus, Frequencies: [", list[ix].name, list[ix].max_sample_length_s);
        for (size_t fx = 0; fx < EDGE_IMPULSE_MAX_FREQUENCIES; fx++) {
            if (list[ix].frequencies[fx] != 0.0f) {
                if (fx != 0) {
                    printf(", ");
                }
                printf("%.2fHz", list[ix].frequencies[fx]);
            }
        }
        printf("]\n");
    }
}

static void at_list_config() {
    printf("===== Device info =====\n");
    at_device_info();
    printf("\n");
    printf("===== Sensors ======\n");
    at_list_sensors();
}


static void at_reset() {
    NVIC_SystemReset();
}

// AT commands related to configuration
void ei_at_register_generic_cmds() {
    ei_at_cmd_register("HELP", "Lists all commands", &ei_at_cmd_print_info);
    ei_at_cmd_register("CONFIG?", "Lists complete config", &at_list_config);
    ei_at_cmd_register("DEVICEINFO?", "Lists device information", &at_device_info);
    ei_at_cmd_register("SENSORS?", "Lists sensors", &at_list_sensors);
    ei_at_cmd_register("RESET", "Reset the system", &at_reset);
}

#endif // _EDGE_IMPULSE_AT_COMMANDS_CONFIG_H_
