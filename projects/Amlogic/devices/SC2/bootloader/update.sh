#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# Update files on the mounted removable or internal LibreELEC boot filesystem.
set -e
BOOT_ROOT="${BOOT_ROOT:-/flash}"
SYSTEM_ROOT="${SYSTEM_ROOT:-}"
SRCDIR="${SYSTEM_ROOT}/usr/share/bootloader"
mount -o remount,rw "${BOOT_ROOT}"
cp -p "${SRCDIR}/meson-sc2-hk1-rbox-x4.dtb" "${BOOT_ROOT}/amlogic/"
cp -p "${SRCDIR}/s905_autoscript" "${SRCDIR}/emmc_autoscript" "${BOOT_ROOT}/"
# Remove the old generated CPUFreq hold instead of carrying it into updates.
# Regenerate live from the migrated normal entry so neither retains off=1;
# leave UUIDs, blkdevparts and the sc2.emmc write opt-in unchanged.
sed -i -E 's/console=ttyAML0,115200n8/console=ttyAML0,921600n8/g; s/(^|[[:space:]])cpufreq\.off=[^[:space:]]+//g' \
  "${BOOT_ROOT}/uEnv.ini"
# The live entry omits persistent storage but retains partition arguments.
sed -E 's/(^|[[:space:]])disk=[^[:space:]]+//g; s/$/ toram live/' \
  "${BOOT_ROOT}/uEnv.ini" > "${BOOT_ROOT}/uEnv.live.ini"
sync
mount -o remount,ro "${BOOT_ROOT}"
