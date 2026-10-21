# SPDX-License-Identifier: GPL-2.0-or-later
PKG_NAME="sc2-optee-preload"
PKG_VERSION="9d6f69844ff60ec0966cf3659abcc38eda8b31ea"
PKG_SHA256="446fb426089ad73405e982d324e59125f6daa5249afe7499efaec6d03b3e2e5e"
PKG_LICENSE="BSD-2-Clause GPL-2.0-or-later"
PKG_SITE="https://github.com/OP-TEE/optee_client"
PKG_URL="${PKG_SITE}/archive/${PKG_VERSION}.tar.gz"
PKG_SOURCE_DIR="optee_client-${PKG_VERSION}"
PKG_DEPENDS_TARGET="toolchain openssl"
PKG_LONGDESC="Verified original local SC2 video initialization with explicit once-per-boot notification and a storage-denying OP-TEE RPC provider"
PKG_TOOLCHAIN="manual"

make_target() {
  # Build only the ordinary client API, never upstream tee-supplicant.
  # In particular, do not define TEEC_LOG_FILE (persistent logging).
  ${CC} ${CFLAGS} -std=c11 -Wall -Wextra -D_GNU_SOURCE \
    -DDEBUGLEVEL_1 -DBINARY_PREFIX=\"SC2\" \
    -I"${PKG_BUILD}/libteec/include" \
    "${PKG_DIR}/src/sc2-optee-preload.c" \
    "${PKG_BUILD}/libteec/src/tee_client_api.c" \
    "${PKG_BUILD}/libteec/src/teec_trace.c" \
    ${LDFLAGS} -pthread -lcrypto -o "${PKG_BUILD}/sc2-optee-preload"
}

makeinstall_target() {
  install -Dm0755 "${PKG_BUILD}/sc2-optee-preload" \
    "${INSTALL}/usr/sbin/sc2-optee-preload"
  install -Dm0644 "${PKG_DIR}/system.d/sc2-optee-preload.service" \
    "${INSTALL}/usr/lib/systemd/system/sc2-optee-preload.service"
}

post_install() {
  enable_service sc2-optee-preload.service
}
