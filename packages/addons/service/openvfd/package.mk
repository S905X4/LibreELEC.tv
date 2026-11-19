# SPDX-License-Identifier: GPL-2.0-or-later

PKG_NAME="openvfd"
PKG_VERSION="c645ccd3a15af9a8bf2ef5048be36b38975abd25"
PKG_SHA256="6f5b99e90b230887084bdde3c6d97473686774535328b258573770d3629532a1"
PKG_REV="0"
PKG_ARCH="aarch64"
PKG_LICENSE="GPL-2.0-or-later"
PKG_SITE="https://github.com/arthur-liberman/service.openvfd"
PKG_URL="https://github.com/arthur-liberman/service.openvfd/archive/${PKG_VERSION}.tar.gz"
PKG_DEPENDS_TARGET="toolchain kodi openvfd-driver xmlstarlet:host"
PKG_SECTION="service"
PKG_SHORTDESC="OpenVFD display service"
PKG_LONGDESC="Upstream OpenVFD Kodi service for display icons, time, date, temperature, playback and brightness settings. Requires the OpenVFD driver and OpenVFDService."
PKG_TOOLCHAIN="manual"
PKG_IS_ADDON="embedded"
PKG_ADDON_NAME="OpenVFD"
PKG_ADDON_TYPE="xbmc.service"

make_target() {
  :
}

makeinstall_target() {
  local dest="${INSTALL}/usr/share/kodi/addons/service.openvfd"
  mkdir -p "${dest}"
  cp -a "${PKG_BUILD}/addon.xml" "${PKG_BUILD}/service.py" \
    "${PKG_BUILD}/icon.png" "${PKG_BUILD}/changelog.txt" \
    "${PKG_BUILD}/LICENSE" "${PKG_BUILD}/resources" "${dest}/"
}

addon() {
  mkdir -p "${ADDON_BUILD}/${PKG_ADDON_ID}"
  cp -a "${PKG_BUILD}/addon.xml" "${PKG_BUILD}/service.py" \
    "${PKG_BUILD}/icon.png" "${PKG_BUILD}/changelog.txt" \
    "${PKG_BUILD}/LICENSE" "${PKG_BUILD}/resources" \
    "${ADDON_BUILD}/${PKG_ADDON_ID}/"
}

post_install() {
  local manifest="${INSTALL}/usr/share/kodi/system/addon-manifest.xml"
  xmlstarlet ed -L -d "/addons/addon[text()='service.openvfd']" \
    --subnode "/addons" -t elem -n addon -v "service.openvfd" \
    --insert "/addons/addon[text()='service.openvfd']" \
    -t attr -n optional -v true "${manifest}"
}
