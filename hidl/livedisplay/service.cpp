/*
 * Copyright (C) 2019 The LineageOS Project
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

#define LOG_TAG "vendor.lineage.livedisplay@2.1-service.universal7870"

#include <android-base/logging.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>

#include "DisplayColorCalibration.h"
#include "DisplayModes.h"
#include "ReadingEnhancement.h"
#include "SunlightEnhancement.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::status_t;
using android::OK;

using vendor::lineage::livedisplay::V2_1::IDisplayColorCalibration;
using vendor::lineage::livedisplay::V2_1::implementation::DisplayColorCalibration;
using vendor::lineage::livedisplay::V2_1::IDisplayModes;
using vendor::lineage::livedisplay::V2_1::implementation::DisplayModes;
using vendor::lineage::livedisplay::V2_1::IReadingEnhancement;
using vendor::lineage::livedisplay::V2_1::implementation::ReadingEnhancement;
using vendor::lineage::livedisplay::V2_1::ISunlightEnhancement;
using vendor::lineage::livedisplay::V2_1::implementation::SunlightEnhancement;

int main() {
    sp<IDisplayColorCalibration> displayColorCalibration;
    sp<IDisplayModes> displayModes;
    sp<IReadingEnhancement> readingEnhancement;
    sp<ISunlightEnhancement> sunlightEnhancement;
    status_t status;

    LOG(INFO) << "LiveDisplay HAL service is starting.";

    displayColorCalibration = new DisplayColorCalibration();
    if (displayColorCalibration == nullptr) {
        LOG(ERROR) << "Can not create an instance of LiveDisplay HAL DisplayColorCalibration "
                      "Iface, exiting.";
        goto shutdown;
    }

    displayModes = new DisplayModes();
    if (displayModes == nullptr) {
        LOG(ERROR) << "Can not create an instance of LiveDisplay HAL DisplayModes "
                      "Iface, exiting.";
        goto shutdown;
    }

    readingEnhancement = new ReadingEnhancement();
    if (readingEnhancement == nullptr) {
        LOG(ERROR)
            << "Can not create an instance of LiveDisplay HAL ReadingEnhancement Iface, exiting.";
        goto shutdown;
    }

    sunlightEnhancement = new SunlightEnhancement();
    if (sunlightEnhancement == nullptr) {
        LOG(ERROR)
            << "Can not create an instance of LiveDisplay HAL SunlightEnhancement Iface, exiting.";
        goto shutdown;
    }

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    status = displayColorCalibration->registerAsService();
    if (status != OK) {
        LOG(ERROR)
            << "Could not register service for LiveDisplay HAL DisplayColorCalibration Iface ("
            << status << ")";
        goto shutdown;
    }

    status = displayModes->registerAsService();
    if (status != OK) {
        LOG(ERROR)
            << "Could not register service for LiveDisplay HAL DisplayColorCalibration Iface ("
            << status << ")";
        goto shutdown;
    }

    status = readingEnhancement->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL ReadingEnhancement Iface ("
                   << status << ")";
        goto shutdown;
    }

    status = sunlightEnhancement->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL SunlightEnhancement Iface ("
                   << status << ")";
        goto shutdown;
    }

    LOG(INFO) << "LiveDisplay HAL service is ready.";
    joinRpcThreadpool();
    // Should not pass this line

shutdown:
    // In normal operation, we don't expect the thread pool to shutdown
    LOG(ERROR) << "LiveDisplay HAL service is shutting down.";
    return 1;
}
