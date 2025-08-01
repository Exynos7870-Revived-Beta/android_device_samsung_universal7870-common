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

#include <android-base/file.h>
#include <android-base/strings.h>

#include <fstream>

#include "SunlightEnhancement.h"

using android::base::ReadFileToString;
using android::base::Trim;
using android::base::WriteStringToFile;

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_1 {
namespace implementation {

static constexpr const char* kLUXPath = "/sys/class/mdnie/mdnie/lux";

Return<bool> SunlightEnhancement::isEnabled() {
    std::string tmp;
    int32_t contents = 0;

    if (ReadFileToString(kLUXPath, &tmp)) {
        contents = std::stoi(Trim(tmp));
    }

    return contents > 0;
}

Return<bool> SunlightEnhancement::setEnabled(bool enabled) {
    /* see drivers/video/fbdev/exynos/decon_7870/panels/mdnie_lite_table*, get_hbm_index */
    return WriteStringToFile(enabled ? "40000" : "0", kLUXPath, true);
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
