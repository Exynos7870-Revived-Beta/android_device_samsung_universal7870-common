/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "ThermalHAL"
#include <log/log.h>

#include <hardware/hardware.h>
#include <hardware/thermal.h>

#define MAX_LENGTH                      50

#define CPU_USAGE_FILE                  "/proc/stat"
#define TEMPERATURE_FILE_FORMAT         "/sys/class/thermal/thermal_zone%d/temp"
#define CPU_ONLINE_FILE_FORMAT          "/sys/devices/system/cpu/cpu%d/online"

const int CPU_SENSORS[] = {1, 1, 1, 1, 0, 0, 0, 0};

#define CPU_NUM                         (sizeof(CPU_SENSORS) / sizeof(int))

#define CPU_SENSOR_NUM                  0
#define GPU_SENSOR_NUM                  2

#define TEMPERATURE_NUM                 4

#define CPU_SHUTDOWN_THRESHOLD          115
#define CPU_THROTTLING_THRESHOLD        86

#define GPU_LABEL                       "GPU"

const char *CPU_LABEL[] = {"CPU0", "CPU1", "CPU2", "CPU3", "CPU4", "CPU5", "CPU6", "CPU7"};
/**
 * Reads device temperature.
 *
 * @param file_name Name of file with temperature.
 * @param temperature_format Format of temperature file.
 * @param type Device temperature type.
 * @param name Device temperature name.
 * @param mult Multiplier used to translate temperature to Celsius.
 * @param throttling_threshold Throttling threshold for the temperature.
 * @param shutdown_threshold Shutdown threshold for the temperature.
 * @param out Pointer to temperature_t structure that will be filled with current values.
 *
 * @return 0 on success or negative value -errno on error.
 */
static ssize_t read_temperature(const char *file_name, const char *temperature_format, int type,
        const char *name, float mult, float throttling_threshold, float shutdown_threshold,
        temperature_t *out) {
    FILE *file;
    float temp;

    file = fopen(file_name, "r");
    if (file == NULL) {
        ALOGE("%s: failed to open: %s", __func__, strerror(errno));
        return -errno;
    }
    if (1 != fscanf(file, temperature_format, &temp)) {
        fclose(file);
        ALOGE("%s: failed to read a float: %s", __func__, strerror(errno));
        return errno ? -errno : -EIO;
    }

    fclose(file);

    (*out) = (temperature_t) {
        .type = type,
        .name = name,
        .current_value = temp * mult,
        .throttling_threshold = throttling_threshold,
        .shutdown_threshold = shutdown_threshold,
        .vr_throttling_threshold = UNKNOWN_TEMPERATURE
    };

    return 0;
}

static ssize_t get_cpu_temperatures(temperature_t *list, size_t size) {
    size_t cpu;

    for (cpu = 0; cpu < CPU_NUM; cpu++) {
        char file_name[MAX_LENGTH];

        if (cpu >= size) {
            break;
        }

        sprintf(file_name, TEMPERATURE_FILE_FORMAT, CPU_SENSORS[cpu]);

        ssize_t result = read_temperature(file_name, "%f", DEVICE_TEMPERATURE_CPU, CPU_LABEL[cpu],
                0.001, CPU_THROTTLING_THRESHOLD, CPU_SHUTDOWN_THRESHOLD, &list[cpu]);
        if (result != 0) {
            return result;
        }
    }
    return cpu;
}

static ssize_t get_temperatures(thermal_module_t *module, temperature_t *list, size_t size) {
    ssize_t result = 0;
    size_t current_index = 0;
    char file_name[MAX_LENGTH];

    if (list == NULL) {
        return TEMPERATURE_NUM;
    }

    result = get_cpu_temperatures(list, size);
    if (result < 0) {
        return result;
    }
    current_index += result;

    // GPU temperautre.
    if (current_index < size) {

        sprintf(file_name, TEMPERATURE_FILE_FORMAT, GPU_SENSOR_NUM);
        result = read_temperature(file_name, "%f", DEVICE_TEMPERATURE_GPU, GPU_LABEL, 0.001,
                UNKNOWN_TEMPERATURE, UNKNOWN_TEMPERATURE, &list[current_index]);
        if (result != 0) {
            return result;
        }
        current_index++;
    }

    return TEMPERATURE_NUM;
}

static ssize_t get_cpu_usages(thermal_module_t *module, cpu_usage_t *list) {
    int vals, cpu_num, online;
    ssize_t read;
    uint64_t user, nice, system, idle, active, total;
    char *line = NULL;
    size_t len = 0;
    size_t size = 0;
    size_t i;
    char file_name[MAX_LENGTH];
    FILE *file;
    FILE *cpu_file;

    if (list == NULL) {
        return CPU_NUM;
    }

    for (i = 0; i < CPU_NUM; i++) {
        list[i].name = CPU_LABEL[i];
        list[i].active = 0;
        list[i].total = 0;
        list[i].is_online = 0;
    }

    file = fopen(CPU_USAGE_FILE, "r");
    if (file == NULL) {
        ALOGE("%s: failed to open: %s", __func__, strerror(errno));
        return -errno;
    }

    while ((read = getline(&line, &len, file)) != -1) {
        // Skip non "cpu[0-9]" lines.
        if (strnlen(line, read) < 4 || strncmp(line, "cpu", 3) != 0 || !isdigit(line[3])) {
            free(line);
            line = NULL;
            len = 0;
            continue;
        }

        vals = sscanf(line, "cpu%d %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64, &cpu_num, &user,
                &nice, &system, &idle);

        free(line);
        line = NULL;
        len = 0;

        if (vals != 5 || size == CPU_NUM) {
            if (vals != 5) {
                ALOGE("%s: failed to read CPU information from file: %s", __func__,
                        strerror(errno));
            } else {
                ALOGE("/proc/stat file has incorrect format.");
            }
            fclose(file);
            return errno ? -errno : -EIO;
        }

        active = user + nice + system;
        total = active + idle;

        // Read online CPU information.
        snprintf(file_name, MAX_LENGTH, CPU_ONLINE_FILE_FORMAT, cpu_num);
        cpu_file = fopen(file_name, "r");
        online = 0;
        if (cpu_file == NULL) {
            ALOGE("%s: failed to open file: %s (%s)", __func__, file_name, strerror(errno));
            fclose(file);
            return -errno;
        }
        if (1 != fscanf(cpu_file, "%d", &online)) {
            ALOGE("%s: failed to read CPU online information from file: %s (%s)", __func__,
                    file_name, strerror(errno));
            fclose(file);
            fclose(cpu_file);
            return errno ? -errno : -EIO;
        }
        fclose(cpu_file);

        list[size] = (cpu_usage_t) {
            .name = CPU_LABEL[size],
            .active = active,
            .total = total,
            .is_online = online
        };

        size++;
    }

    fclose(file);

    if (size > CPU_NUM) {
        ALOGE("/proc/stat file has incorrect format.");
        return -EIO;
    }

    return CPU_NUM;
}

static struct hw_module_methods_t thermal_module_methods = {
    .open = NULL,
};

thermal_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = THERMAL_HARDWARE_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = THERMAL_HARDWARE_MODULE_ID,
        .name = "Exynos7870 Thermal HAL",
        .author = "The Android Open Source Project",
        .methods = &thermal_module_methods,
    },
    .getTemperatures = get_temperatures,
    .getCpuUsages = get_cpu_usages,
};


