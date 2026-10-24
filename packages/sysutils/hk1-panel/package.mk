# SPDX-License-Identifier: GPL-2.0-or-later
PKG_NAME="hk1-panel"
PKG_VERSION="1.0"
PKG_LICENSE="GPL-2.0-or-later"
PKG_DEPENDS_TARGET="toolchain"
PKG_LONGDESC="Native HK1 RBOX X4 clock and LAN/WLAN/USB indicator policy"
PKG_TOOLCHAIN="manual"

make_target() {
  ${CC} ${CFLAGS} -std=c11 -Wall -Wextra \
    "${PKG_DIR}/src/hk1-panel.c" ${LDFLAGS} -o "${PKG_BUILD}/hk1-panel"
}

makeinstall_target() {
  mkdir -p "${INSTALL}/usr/sbin"
  install -m 0755 "${PKG_BUILD}/hk1-panel" "${INSTALL}/usr/sbin/hk1-panel"
}

post_install() {
  enable_service hk1-panel.service
}
