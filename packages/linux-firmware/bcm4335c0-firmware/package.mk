# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 hiper25 <whydonit@gmail.com>

PKG_NAME="bcm4335c0-firmware"
PKG_VERSION="35e36956d7707fc86bbb48ed70ddb6289e2bbafd"
PKG_SHA256="7f60c8874b5e776801c38c60a877e8af55052554a8e688585efa5fd45ff4baf2"
PKG_LICENSE="other"
PKG_SITE="https://github.com/OpenELEC/wlan-firmware"
PKG_URL="${PKG_SITE}/archive/${PKG_VERSION}.tar.gz"
PKG_SOURCE_DIR="wlan-firmware-${PKG_VERSION}"
PKG_DEPENDS_TARGET="toolchain"
PKG_LONGDESC="Broadcom BCM4335C0 UART Bluetooth patch with its redistribution terms"
PKG_TOOLCHAIN="manual"

makeinstall_target() {
  # ROM subversion 0x6109 requests BCM4335C0.hcd via the native btbcm driver.
  install -Dm0644 firmware/brcm/bcm4335c0.hcd \
    "${INSTALL}/$(get_full_firmware_dir)/brcm/BCM4335C0.hcd"
  install -Dm0644 firmware/brcm/LICENCE.broadcom_bcm43xx \
    "${INSTALL}/usr/share/licenses/${PKG_NAME}/LICENCE.broadcom_bcm43xx"
}
