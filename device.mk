# Copyright (C) 2014 The CyanogenMod Project
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

LOCAL_PATH := device/samsung/pontesolo

# Overlay
DEVICE_PACKAGE_OVERLAYS += $(LOCAL_PATH)/overlay

# Boot animation
TARGET_SCREEN_HEIGHT := 480
TARGET_SCREEN_WIDTH  := 360

# Proprietary files
$(call inherit-product, vendor/samsung/pontesolo/pontesolo-vendor.mk)

# Audio configuration
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/audio_platform_info.xml:system/etc/audio_platform_info.xml \
    $(LOCAL_PATH)/audio/audio_policy.conf:system/etc/audio_policy.conf \
    $(LOCAL_PATH)/audio/mixer_paths.xml:system/etc/mixer_paths.xml


# Doze
PRODUCT_PACKAGES += \
    SamsungDoze

# Lights
PRODUCT_PACKAGES += \
    lights.msm8226

# Media
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/media_profiles.xml:system/etc/media_profiles.xml

# SWAP create-activate script
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/swap.sh:system/etc/swap.sh

# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml

# Ramdisk
PRODUCT_PACKAGES += \
    init.crda.sh \
    init.ponte-solo.bt.sh

PRODUCT_PACKAGES += \
    fstab.qcom \
    init.qcom.rc \
    init.qcom.usb.rc \
    ueventd.qcom.rc

# Thermal
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/thermal-engine-8226.conf:system/etc/thermal-engine-8226.conf

# Wifi
PRODUCT_PACKAGES += \
    libnetcmdiface \
    macloader \
    hostapd_default.conf \
    p2p_supplicant_overlay.conf \
    wpa_supplicant_overlay.conf

PRODUCT_COPY_FILES += \
   $(LOCAL_PATH)/configs/wpa_supplicant_overlay.conf:system/etc/wifi/wpa_supplicant_overlay.conf \
   $(LOCAL_PATH)/configs/p2p_supplicant_overlay.conf:system/etc/wifi/p2p_supplicant_overlay.conf

# Inherit from qcom-common
$(call inherit-product, device/samsung/msm8226-common/msm8226.mk)
