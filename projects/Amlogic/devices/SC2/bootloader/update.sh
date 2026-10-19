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
sync
mount -o remount,ro "${BOOT_ROOT}"
