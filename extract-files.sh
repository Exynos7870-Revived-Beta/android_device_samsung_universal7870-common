#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

ONLY_COMMON=
ONLY_TARGET=
KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common )
                ONLY_COMMON=true
                ;;
        --only-target )
                ONLY_TARGET=true
                ;;
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -k | --kang )
                KANG="--kang"
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
                SRC="${1}"
                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

function blob_fixup() {
    case "${1}" in
        vendor/bin/hw/rild)
            "${PATCHELF}" --replace-needed "libril.so" "libril-samsung.so" "${2}"
            ;;
        vendor/lib/hw/audio.primary.exynos7870.so)
            sed -i 's|libtinyalsa.so|libalsa7870.so|g' "${2}"
            sed -i 's|libaudioroute.so|libaudior7870.so|g' "${2}"
            ;;
        vendor/lib/libpreprocessing_nxp.so)
            sed -i 's|libtinyalsa.so|libalsa7870.so|g' "${2}"
            ;;
        vendor/lib/libaudior7870.so)
            sed -i 's|libtinyalsa.so|libalsa7870.so|g' "${2}"
            sed -i 's|libaudioroute.so|libaudior7870.so|g' "${2}"
            ;;
        vendor/lib/libalsa7870.so)
            sed -i 's|libtinyalsa.so|libalsa7870.so|g' "${2}"
            ;;
        vendor/lib/hw/memtrack.exynos7870.so)
            sed -i 's|memtrack.universal7880.so|memtrack.universal7870.so|g' "${2}"
            ;;
        vendor/lib/hw/hwcomposer.exynos7870.so)
            sed -i 's|\x73\x79\x73\x2F\x64\x65\x76\x69\x63\x65\x73\x2F\x00\x31\x34\x38\x33\x30\x30\x30\x30\x2E\x64\x65\x63\x6F\x6E\x5F\x66\x2F\x76\x73\x79\x6E\x63\x00\x31\x34\x38\x36\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x31\x34\x38\x36\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x00\x65\x78\x79\x6E\x6F\x73\x35\x2D\x66\x62\x2E\x31\x2F\x76\x73\x79\x6E\x63\x00\x70\x6C\x61\x74\x66\x6F\x72\x6D\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x33\x30\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x31\x31\x2F\x00\x66\x61\x69\x6C\x65\x64\x20|\x73\x79\x73\x2F\x64\x65\x76\x69\x63\x65\x73\x2F\x00\x31\x34\x38\x33\x30\x30\x30\x30\x2E\x64\x65\x63\x6F\x6E\x5F\x66\x62\x2F\x76\x73\x79\x6E\x63\x00\x31\x34\x38\x35\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x31\x34\x38\x35\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x00\x65\x78\x79\x6E\x6F\x73\x35\x2D\x66\x62\x2E\x31\x2F\x76\x73\x79\x6E\x63\x00\x70\x6C\x61\x74\x66\x6F\x72\x6D\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x33\x30\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x31\x31\x2F\x00\x66\x61\x69\x6C\x64\x20|g' "${2}"
            ;;
        vendor/lib/libExynosOMX_Core.so)
            sed -i 's|system/lib|vendor/lib|g' "${2}"
            ;;
        vendor/lib/omx/libOMX.Exynos.AVC.Decoder.so)
            sed -i 's|system/lib|vendor/lib|g' "${2}"
            ;;
        vendor/lib/omx/libOMX.Exynos.VP9.Decoder.so)
            sed -i 's|system/lib|vendor/lib|g' "${2}"
            ;;
        vendor/lib/omx/libOMX.Exynos.HEVC.Decoder.so)
            sed -i 's|system/lib|vendor/lib|g' "${2}"
            ;;
        vendor/lib/omx/libOMX.Exynos.WMV.Decoder.so)
            sed -i 's|system/lib|vendor/lib|g' "${2}"
            ;;
        vendor/lib/omx/libOMX.Exynos.VP8.Decoder.so)
            sed -i 's|system/lib|vendor/lib|g' "${2}"
            ;;
        vendor/lib/omx/libOMX.Exynos.MPEG4.Decoder.so)
            sed -i 's|system/lib|vendor/lib|g' "${2}"
            ;;
        vendor/lib/libsec-ril.so)
            "${PATCHELF}" --replace-needed "libril.so" "libril-samsung.so" "${2}"
            "${PATCHELF}" --add-needed "libcutils_shim_vendor.so" "${2}"
            ;;
        vendor/lib/libsec-ril-dsds.so)
            "${PATCHELF}" --replace-needed "libril.so" "libril-samsung.so" "${2}"
            "${PATCHELF}" --add-needed "libcutils_shim_vendor.so" "${2}"
            ;;
        vendor/lib/hw/camera.vendor.exynos7870.so)
            "${PATCHELF}" --replace-needed "libcamera_client.so" "libcamera_metadata_helper.so" "${2}"
            "${PATCHELF}" --replace-needed "libgui.so" "libgui_vendor.so" "${2}"
            "${PATCHELF}" --add-needed "libexynoscamera_shim.so" "${2}"
            ;;
        vendor/lib/libsensorlistener.so)
            "${PATCHELF}" --add-needed "libshim_sensorndkbridge.so" "${2}"
            ;;
        vendor/lib/libwvhidl.so)
            "${PATCHELF}" --replace-needed "libprotobuf-cpp-lite.so" "libprotobuf-cpp-lite-v29.so" "${2}"
            ;;
        vendor/lib/mediadrm/libwvdrmengine.so)
            "${PATCHELF}" --replace-needed "libprotobuf-cpp-lite.so" "libprotobuf-cpp-lite-v29.so" "${2}"
            ;;
        vendor/lib64/hw/memtrack.exynos7870.so)
            sed -i 's|memtrack.universal7880.so|memtrack.universal7870.so|g' "${2}"
            ;;
        vendor/lib64/hw/hwcomposer.exynos7870.so)
            sed -i 's|\x73\x79\x73\x2F\x64\x65\x76\x69\x63\x65\x73\x2F\x00\x31\x34\x38\x33\x30\x30\x30\x30\x2E\x64\x65\x63\x6F\x6E\x5F\x66\x2F\x76\x73\x79\x6E\x63\x00\x31\x34\x38\x36\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x31\x34\x38\x36\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x00\x65\x78\x79\x6E\x6F\x73\x35\x2D\x66\x62\x2E\x31\x2F\x76\x73\x79\x6E\x63\x00\x70\x6C\x61\x74\x66\x6F\x72\x6D\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x33\x30\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x31\x31\x2F\x00\x66\x61\x69\x6C\x65\x64\x20|\x73\x79\x73\x2F\x64\x65\x76\x69\x63\x65\x73\x2F\x00\x31\x34\x38\x33\x30\x30\x30\x30\x2E\x64\x65\x63\x6F\x6E\x5F\x66\x62\x2F\x76\x73\x79\x6E\x63\x00\x31\x34\x38\x35\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x31\x34\x38\x35\x30\x30\x30\x30\x2E\x73\x79\x73\x6D\x6D\x75\x2F\x00\x65\x78\x79\x6E\x6F\x73\x35\x2D\x66\x62\x2E\x31\x2F\x76\x73\x79\x6E\x63\x00\x70\x6C\x61\x74\x66\x6F\x72\x6D\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x33\x30\x2F\x65\x78\x79\x6E\x6F\x73\x2D\x73\x79\x73\x6D\x6D\x75\x2E\x31\x31\x2F\x00\x66\x61\x69\x6C\x64\x20|g' "${2}"
            ;;
        vendor/lib64/libExynosOMX_Core.so)
            sed -i 's|system/lib64|vendor/lib64|g' "${2}"
            ;;
        vendor/lib64/omx/libOMX.Exynos.AVC.Decoder.so)
            sed -i 's|system/lib64|vendor/lib64|g' "${2}"
            ;;
        vendor/lib64/omx/libOMX.Exynos.VP9.Decoder.so)
            sed -i 's|system/lib64|vendor/lib64|g' "${2}"
            ;;
        vendor/lib64/omx/libOMX.Exynos.HEVC.Decoder.so)
            sed -i 's|system/lib64|vendor/lib64|g' "${2}"
            ;;
        vendor/lib64/omx/libOMX.Exynos.WMV.Decoder.so)
            sed -i 's|system/lib64|vendor/lib64|g' "${2}"
            ;;
        vendor/lib64/omx/libOMX.Exynos.VP8.Decoder.so)
            sed -i 's|system/lib64|vendor/lib64|g' "${2}"
            ;;
        vendor/lib64/omx/libOMX.Exynos.MPEG4.Decoder.so)
            sed -i 's|system/lib64|vendor/lib64|g' "${2}"
            ;;
        vendor/lib64/libsec-ril.so)
            "${PATCHELF}" --replace-needed "libril.so" "libril-samsung.so" "${2}"
            "${PATCHELF}" --add-needed "libcutils_shim_vendor.so" "${2}"
            ;;
        vendor/lib64/libsec-ril-dsds.so)
            "${PATCHELF}" --replace-needed "libril.so" "libril-samsung.so" "${2}"
            "${PATCHELF}" --add-needed "libcutils_shim_vendor.so" "${2}"
            ;;
    esac
}

if [ -z "${ONLY_TARGET}" ]; then
    # Initialize the helper for common device
    setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

    extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

"${MY_DIR}/setup-makefiles.sh"
