#
# Copyright (C) 2023 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
   service.cpp \

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libdl \
	libutils \
	libhidlbase \
	android.hardware.sensors@1.0

LOCAL_MODULE := android.hardware.sensors@1.0-service.universal7870
LOCAL_INIT_RC := android.hardware.sensors@1.0-service.universal7870.rc
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH:= arm
LOCAL_VENDOR_MODULE := true

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
   Sensors.cpp

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libhardware \
	libbase \
	libutils \
	libhidlbase \
	android.hardware.sensors@1.0

LOCAL_STATIC_LIBRARIES := \
	android.hardware.sensors@1.0-convert \
	multihal

LOCAL_MODULE := android.hardware.sensors@1.0-impl.universal7870
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH:= arm
LOCAL_VENDOR_MODULE := true


include $(BUILD_SHARED_LIBRARY)
