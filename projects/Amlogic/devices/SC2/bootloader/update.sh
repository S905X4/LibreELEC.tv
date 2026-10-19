#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
# Updates are restricted to files on the mounted removable boot partition.
set -e
BOOT_ROOT="${BOOT_ROOT:-/flash}"
SYSTEM_ROOT="${SYSTEM_ROOT:-}"
SRCDIR="${SYSTEM_ROOT}/usr/share/bootloader"
mount -o remount,rw "${BOOT_ROOT}"
cp -p "${SRCDIR}/meson-sc2-hk1-rbox-x4.dtb" "${BOOT_ROOT}/amlogic/"
cp -p "${SRCDIR}/s905_autoscript" "${BOOT_ROOT}/"
sed -i 's/console=ttyAML0,115200n8/console=ttyAML0,921600n8/g' "${BOOT_ROOT}/uEnv.ini"
# A separate live entry frees the card without changing normal storage UUIDs.
sed -E 's/(^|[[:space:]])disk=[^[:space:]]+//g; s/$/ toram live/' \
  "${BOOT_ROOT}/uEnv.ini" > "${BOOT_ROOT}/uEnv.live.ini"
sync
mount -o remount,ro "${BOOT_ROOT}"
