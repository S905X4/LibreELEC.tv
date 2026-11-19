# SPDX-License-Identifier: GPL-2.0-or-later
PKG_NAME="openvfd-driver"
PKG_VERSION="775bdf8c0f6e959a6e9de043ee6838fe947994c6"
PKG_SHA256="24f8a8a36ae03d0e212af8445dd34763052fbf7ed883c1650e70386618f575e9"
PKG_LICENSE="GPL-3.0"
PKG_SITE="https://github.com/arthur-liberman/linux_openvfd"
PKG_URL="https://github.com/arthur-liberman/linux_openvfd/archive/${PKG_VERSION}.tar.gz"
PKG_LONGDESC="Upstream OpenVFD kernel driver and 24-hour clock service"
PKG_IS_KERNEL_PKG="yes"
PKG_TOOLCHAIN="manual"

make_target() {
  kernel_make -C "$(kernel_path)" M="${PKG_BUILD}/driver" modules
  ${CC} ${CFLAGS} -o "${PKG_BUILD}/OpenVFDService" \
    "${PKG_BUILD}/OpenVFDService.c" ${LDFLAGS} -lm -pthread
}

makeinstall_target() {
  mkdir -p "${INSTALL}/$(get_full_module_dir)/${PKG_NAME}"
  install -m 0644 "${PKG_BUILD}/driver/openvfd.ko" \
    "${INSTALL}/$(get_full_module_dir)/${PKG_NAME}/openvfd.ko"
  mkdir -p "${INSTALL}/usr/sbin"
  install -m 0755 "${PKG_BUILD}/OpenVFDService" "${INSTALL}/usr/sbin/OpenVFDService"
}

post_install() {
  enable_service openvfd.service
}
