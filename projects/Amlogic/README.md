# Amlogic

This project is for Amlogic SoC devices

## Links

* http://linux-meson.com

## Useful debug commands

* `cat /sys/kernel/debug/cec/cec0/status`
* `cat /sys/kernel/debug/clk/clk_summary`
* `cat /sys/class/drm/card0-HDMI-A-1/status`
* `hexdump -C /sys/class/drm/card0-HDMI-A-1/edid`
* `edid-decode /sys/class/drm/card0-HDMI-A-1/edid`
* `cat /sys/kernel/debug/dma_buf/bufinfo`

## HK1 RBOX X4 (S905X4 / SC2)

This downstream target is based on LibreELEC 12.2.1 and Linux 6.18.57.
It uses the manufacturer's existing U-Boot and is intended for removable
USB or TF boot. It is not an official LibreELEC device release.

Build on a host with the LibreELEC build dependencies and GCC 12:

```sh
LOCAL_CC=gcc-12 LOCAL_CXX=g++-12 PROJECT=Amlogic DEVICE=SC2 ARCH=aarch64 make image
```

The source includes the SC2 kernel, FFmpeg and Kodi overlays. The prior
6.18.57 USB image was tested on the 4 GB HK1 RBOX X4 with basic HDMI/audio,
network, storage and hardware playback. The public checkout was verified
through package unpack and patch application, not a fresh image or board boot.

No factory ROM, private firmware or calibration is included. Hardware
decoding may need the owner's legally available factory video assets.
The device-bound eMMC installer is omitted, and SSH is not forced on boot.
Keep the original boot firmware and do not flash a removable image to eMMC.

WMV3 tail output, some AVS streams, AV1 film grain, extended codec coverage,
sustained playback and the television-to-ARC audio path remain unverified
or incomplete.
