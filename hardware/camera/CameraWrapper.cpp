/*
 * Copyright (C) 2017, The LineageOS Project
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

#define LOG_NDEBUG 0
#define LOG_PARAMETERS

#define LOG_TAG "CameraWrapper"
#include <cutils/log.h>
#include <android/fdsan.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <system/camera_metadata.h>
#include "CameraWrapper.h"
#include "Camera2Wrapper.h"
#include "Camera3Wrapper.h"
#include "SECCameraProperties.h"


static camera_module_t *gVendorModule = 0;

static int camera_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);
static int camera_set_callbacks(const camera_module_callbacks_t *callbacks);
static void camera_get_vendor_tag_ops(vendor_tag_ops_t* ops);
static int camera_open_legacy(const struct hw_module_t* module, const char* id,
        uint32_t halVersion, struct hw_device_t** device);
static int camera_set_torch_mode(const char* camera_id, bool enabled);
static int camera_init();

static int check_vendor_module()
{
    android_fdsan_set_error_level(ANDROID_FDSAN_ERROR_LEVEL_DISABLED);
    int rv = 0;
    ALOGI("%s: checking vendor module...", __FUNCTION__);

    if(gVendorModule)
        return 0;

    rv = hw_get_module_by_class("camera", "vendor", (const hw_module_t **)&gVendorModule);
    if (rv)
        ALOGE("failed to open vendor camera module: %d (%s)", rv, strerror(-rv));
    else
        ALOGI("opened vendor camera module successfully: %s", gVendorModule->common.name);
    return rv;
}

static struct hw_module_methods_t camera_module_methods = {
    .open = camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
         .tag = HARDWARE_MODULE_TAG,
         .module_api_version = CAMERA_MODULE_API_VERSION_2_4,
         .hal_api_version = HARDWARE_HAL_API_VERSION,
         .id = CAMERA_HARDWARE_MODULE_ID,
         .name = "Samsung Camera Wrapper",
         .author = "The LineageOS Project",
         .methods = &camera_module_methods,
         .dso = NULL,
         .reserved = {0},
    },
    .get_number_of_cameras = camera_get_number_of_cameras,
    .get_camera_info = camera_get_camera_info,
    .set_callbacks = camera_set_callbacks,
    .get_vendor_tag_ops = camera_get_vendor_tag_ops,
    .open_legacy = camera_open_legacy,
    .set_torch_mode = camera_set_torch_mode, 
    .init = camera_init,
    .reserved = {0},
};

static int camera_device_open(const hw_module_t* module, const char* name,
                hw_device_t** device)
{
    int rv = -EINVAL;

    if (name != NULL) {
        if (check_vendor_module())
            return -EINVAL;
        rv = camera3_device_open(module, name, device);
    }

    return rv;
}

static int camera_get_number_of_cameras(void)
{
    ALOGI("%s: enter", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    int num = gVendorModule->get_number_of_cameras();
    ALOGI("%s: vendor returned %d cameras", __FUNCTION__, num);
    return num;
}

static const camera_module_callbacks_t *gCameraCallbacks = NULL;
static camera_metadata_t *gFrontCameraCharacteristics = NULL;

int set_front_torch_state(bool enabled) {
    const char *paths[] = {
        "/sys/class/camera/flash/front_torch_flash",
        "/sys/class/camera/flash/front_flash",
    };
    int fd = -1;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        fd = open(paths[i], O_WRONLY);
        if (fd >= 0) break;
    }
    if (fd < 0) {
        ALOGE("%s: failed to open front torch sysfs node: %s", __FUNCTION__, strerror(errno));
        return -errno;
    }
    const char *val = enabled ? "1" : "0";
    write(fd, val, 1);
    close(fd);
    return 0;
}

void camera_notify_torch_status(int camera_id, int new_status) {
    if (gCameraCallbacks && gCameraCallbacks->torch_mode_status_change) {
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "%d", camera_id);
        gCameraCallbacks->torch_mode_status_change(gCameraCallbacks, id_str, new_status);
    }
}

static camera_metadata_t *create_augmented_camera_info_metadata(const camera_metadata_t *src) {
    if (!src) return NULL;
    size_t entry_count = get_camera_metadata_entry_count(src);
    size_t data_count = get_camera_metadata_data_count(src);

    camera_metadata_t *dst = allocate_camera_metadata(entry_count + 12, data_count + 256);
    if (!dst) return NULL;

    if (append_camera_metadata(dst, src) != 0) {
        ALOGE("%s: append_camera_metadata failed", __FUNCTION__);
        free_camera_metadata(dst);
        return NULL;
    }

    camera_metadata_entry_t entry;
    uint8_t flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    if (find_camera_metadata_entry(dst, ANDROID_FLASH_INFO_AVAILABLE, &entry) == 0) {
        update_camera_metadata_entry(dst, entry.index, &flashAvailable, 1, NULL);
    } else {
        add_camera_metadata_entry(dst, ANDROID_FLASH_INFO_AVAILABLE, &flashAvailable, 1);
    }

    int64_t chargeDuration = 0;
    if (find_camera_metadata_entry(dst, ANDROID_FLASH_INFO_CHARGE_DURATION, &entry) == 0) {
        update_camera_metadata_entry(dst, entry.index, &chargeDuration, 1, NULL);
    } else {
        add_camera_metadata_entry(dst, ANDROID_FLASH_INFO_CHARGE_DURATION, &chargeDuration, 1);
    }

    uint8_t colorTemp = 0;
    if (find_camera_metadata_entry(dst, ANDROID_FLASH_COLOR_TEMPERATURE, &entry) == 0) {
        update_camera_metadata_entry(dst, entry.index, &colorTemp, 1, NULL);
    } else {
        add_camera_metadata_entry(dst, ANDROID_FLASH_COLOR_TEMPERATURE, &colorTemp, 1);
    }

    uint8_t maxEnergy = 0;
    if (find_camera_metadata_entry(dst, ANDROID_FLASH_MAX_ENERGY, &entry) == 0) {
        update_camera_metadata_entry(dst, entry.index, &maxEnergy, 1, NULL);
    } else {
        add_camera_metadata_entry(dst, ANDROID_FLASH_MAX_ENERGY, &maxEnergy, 1);
    }

    // Ensure AE available modes include ON_AUTO_FLASH and ON_ALWAYS_FLASH
    if (find_camera_metadata_entry(dst, ANDROID_CONTROL_AE_AVAILABLE_MODES, &entry) == 0) {
        bool hasAutoFlash = false, hasAlwaysFlash = false;
        for (size_t i = 0; i < entry.count; i++) {
            if (entry.data.u8[i] == ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH) hasAutoFlash = true;
            if (entry.data.u8[i] == ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH) hasAlwaysFlash = true;
        }
        if (!hasAutoFlash || !hasAlwaysFlash) {
            uint8_t aeModes[] = {
                ANDROID_CONTROL_AE_MODE_OFF,
                ANDROID_CONTROL_AE_MODE_ON,
                ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH,
                ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH
            };
            update_camera_metadata_entry(dst, entry.index, aeModes, sizeof(aeModes), NULL);
        }
    }

    ALOGI("%s: successfully augmented front camera metadata with flash availability", __FUNCTION__);
    return dst;
}

static int camera_get_camera_info(int camera_id, struct camera_info *info)
{
    ALOGI("%s: camera_id=%d", __FUNCTION__, camera_id);
    if (check_vendor_module())
        return 0;
    int ret = gVendorModule->get_camera_info(camera_id, info);
    if (ret == 0 && info && camera_id == 1) {
        if (!gFrontCameraCharacteristics && info->static_camera_characteristics) {
            gFrontCameraCharacteristics = create_augmented_camera_info_metadata(info->static_camera_characteristics);
        }
        if (gFrontCameraCharacteristics) {
            info->static_camera_characteristics = gFrontCameraCharacteristics;
        }
    }
    ALOGI("%s: camera_id=%d, facing=%d, orientation=%d, device_version=0x%x, ret=%d",
          __FUNCTION__, camera_id, info ? info->facing : -1, info ? info->orientation : -1,
          info ? info->device_version : 0, ret);
    return ret;
}

static int camera_set_callbacks(const camera_module_callbacks_t *callbacks)
{
    ALOGV("%s", __FUNCTION__);
    gCameraCallbacks = callbacks;
    if (check_vendor_module())
        return 0;
    int ret = gVendorModule->set_callbacks(callbacks);
    camera_notify_torch_status(1, TORCH_MODE_STATUS_AVAILABLE_OFF);
    return ret;
}

static void camera_get_vendor_tag_ops(vendor_tag_ops_t* ops)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return;
    return gVendorModule->get_vendor_tag_ops(ops);
}

static int camera_open_legacy(const struct hw_module_t* module, const char* id, uint32_t halVersion __unused, struct hw_device_t** device)
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return camera2_device_open(module, id, device);
}

static int camera_set_torch_mode(const char* camera_id, bool enabled)
{
    ALOGI("%s: camera_id=%s, enabled=%d", __FUNCTION__, camera_id, enabled);
    if (check_vendor_module())
        return 0;

    int id = atoi(camera_id);
    if (id == 1) {
        int res = set_front_torch_state(enabled);
        if (res == 0) {
            camera_notify_torch_status(1, enabled ? TORCH_MODE_STATUS_AVAILABLE_ON : TORCH_MODE_STATUS_AVAILABLE_OFF);
        }
        return res;
    }

    return gVendorModule->set_torch_mode(camera_id, enabled);
}

static int camera_init()
{
    ALOGV("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return gVendorModule->init();
}
