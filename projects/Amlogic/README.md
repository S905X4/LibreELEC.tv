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

## SC2 / HK1 RBOX X4 minimal bring-up

Build from the LibreELEC root as a non-root user with the normal LE host
dependencies and GCC 12 available on `PATH`:

```sh
LOCAL_CC=gcc-12 LOCAL_CXX=g++-12 PROJECT=Amlogic DEVICE=SC2 ARCH=aarch64 make image
```

This selects `LINUX=amlogic-sc2`, Cortex-A55, and the pinned Linux 6.18.57
recipe. It does not select Kodi, skins, the settings addon, Mesa, FFmpeg,
PulseAudio, or PipeWire. The independent `AMLGX` device is unchanged.
The kernel includes its gzip-compressed initramfs inside an **uncompressed
legacy uImage**; there is no separate initrd to load. SD, USB host/storage,
the serial console, and wired-network drivers are built in. Optional native
wireless and USB Ethernet drivers are modules; firmware inclusion is not
evidence that an onboard wireless module works.

GCC 12 is used for host tools: this baseline's ccache 4.8.3 failed with
GCC 14's `dangling-reference` diagnostic in its bundled fmt. The target
cross-compiler is still built by LE. SC2-specific compatibility patches fix
the Linux 6.18.57 exported vhost header and systemd 255's older vendored
annotation definition. strace 6.16 uses its supported `--enable-bundled=yes`
UAPI mode; this does not upgrade its syscall/ioctl decoders to a newer release.

### Removable boot only

Write the resulting image to an SD card or USB drive, **not eMMC**. The image
contains `KERNEL`, `SYSTEM`, `uEnv.ini`, `s905_autoscript`, and the preselected
`amlogic/meson-sc2-hk1-rbox-x4.dtb`. There is no U-Boot/FIP binary, activation
script, `installtointernal`, environment installer, or raw bootloader write.
In particular, the AMLGX `aml_autoscript` that runs `defenv`/`saveenv` is not
included. The kernel board DT leaves eMMC disabled.

On a stock bootloader without an already-enabled removable autoscript path,
interrupt vendor U-Boot over UART and invoke the script explicitly. First use
`mmc list` to identify the removable SD device. Linux aliases do **not** establish
vendor U-Boot numbering. In the following **RAM-only** commands, replace
`SD_DEVICE_NUMBER` with that observed number; do not select the eMMC device:

```text
setenv sc2_mmcdev SD_DEVICE_NUMBER
mmc dev ${sc2_mmcdev}
mmc rescan
fatload mmc ${sc2_mmcdev}:1 0x22000000 s905_autoscript
autoscr 0x22000000
```

For USB, substitute `usb start` and
`fatload usb 0:1 0x22000000 s905_autoscript`. Use `source` instead of `autoscr`
if that is the command exposed by the installed vendor U-Boot. Do not invoke
the last command if the load failed. Do not run `saveenv`, `defenv`, recovery
activation, or an eMMC installation command. The stock firmware's automatic
script discovery is not assumed or modified; removing the medium and resetting
returns to its original boot policy.

The script tries SD only when `sc2_mmcdev` has been explicitly set, then tries
USB devices `0:1` through `3:1`. There is no default MMC scan or assumed eMMC
number. It imports only `bootargs` from `uEnv.ini`, then loads the fixed HK1 DTB.
The image supplies unique partition UUIDs for `boot=` and `disk=`, not Android
partitions. Load failures return to vendor U-Boot.
Boot-file updates replace the DTB and script on the mounted `/flash`, and migrate
the old `console=ttyAML0,115200n8` in `uEnv.ini` to `console=ttyAML0,921600n8`.
The existing partition UUIDs and other boot arguments are preserved; do not copy
a freshly generated image's `uEnv.ini` onto partitions with different UUIDs.

Kernel placement follows Linux 6.18's arm64 boot protocol: the Image has zero
`text_offset` and needs a 2 MiB-aligned load/entry address. Buffers avoid the
stock/hkallweit memory reservations; this is **not a successful hardware boot**:

| Object | Address / constraint |
| --- | --- |
| uImage staging buffer | `0x12000000` |
| Kernel payload + embedded initramfs | load/entry `0x02000000` |
| Maximum uImage file size | strictly below `0x03000000` (checked before `bootm`) |
| DTB, with 64 KiB expansion space | `0x20000000` (in-place, relocation disabled) |
| Temporary `uEnv.ini` input | `0x21000000` |
| Manual script load | `0x22000000` |

The payload limit keeps it below the secure reservation starting at
`0x05000000`; staging also avoids that reservation and `0x10000000` secure
memory. No second initramfs address is needed: `bootm KERNEL_ADDRESS - DTB_ADDRESS`
uses the initramfs embedded in the kernel. A changed image or firmware memory
map requires rechecking these bounds before booting.

### Login, network, and observations

The command line selects `ttyAML0` at **921600 8N1**, DT-selected `earlycon`,
and `ssh`. LE's vendor `multi-user.target.wants` link pulls in
`sc2-console.service`, running util-linux `agetty` and BusyBox `login`, not
an autologin or a debug shell. ConnMan
starts without the Kodi wizard and enables wired DHCP. OpenSSH starts via
the existing `ssh` command-line condition, creates persistent host keys under
`/storage/.cache/ssh`, and includes the SFTP server. Initial credentials follow
LE's existing convention: **root / libreelec**. Change the password immediately
with `passwd` (stored persistently in `/storage/.cache/shadow`) and use an
isolated bring-up network until then. UART login requires the same password.
No `systemd.debug_shell` argument is supplied; initramfs failures stop with a
diagnostic instead of starting an unauthenticated shell.

The owner confirms this box's TTL console runs at 921600 baud. Keep the host
terminal, kernel `console=`, DT `stdout-path`, and `agetty` at that rate.
Earlycon inherits the firmware's UART setup; the former 115200 setting changed
the baud when the full driver took over, rather than preserving the working rate.

LE's vendor `.wants` links can start a service even when `systemctl is-enabled`
reports `disabled`; inspect runtime state with `systemctl is-active
sc2-console.service`.

Useful headless observations include `uname -a`, `cat /proc/device-tree/model`,
`dmesg`, `journalctl -b`, `ip address`, `connmanctl services`, `ethtool eth0`,
`lsmod`, `strace`, and `/sys/kernel/debug/clk/clk_summary`. The image also includes
`dtc`, `memtester`, `gdb`, `usbutils`, `pciutils`, and `nano`.

This target intentionally disables DRM, GPU, media, and sound in the kernel.
An initial HK1 kernel log is not evidence of working HDMI or a complete board
boot. The normal distro-wide kernel-options audit includes desktop/container/media
requirements that this bring-up configuration intentionally does not satisfy;
review diagnostics in that context rather than enabling the multimedia stack.
CPU DVFS and thermal safety under sustained load remain unverified.
Do not treat an initial boot or a userspace simulation as a stress-test result.

### Verification and limits

The complete `make image` build succeeded, including the removable disk image.
The resulting image passed its SHA-256 check. Its FAT-shipped HK1 DTB passed
`dt-validate` without diagnostics, and the legacy uImage has the documented
`0x02000000` load/entry address. The gap between the generated MBR and the first
partition contains no raw U-Boot/FIP payload.

The initial image was exercised in two independent QEMU boots using its FAT
partition, `SYSTEM`, and production initramfs. This used a separate Linux 6.18.57
diagnostic kernel, built with LE's target compiler and the production configuration
plus QEMU PL011/VirtIO support, with the production initramfs supplied externally.
A VM-only udev rule mapped PL011 to `ttyAML0` to exercise the shipped console unit;
neither the diagnostic kernel nor that rule is included in the deliverable.

Observed runtime results:

* Read-only `/flash` and SquashFS root, writable ext4 `/storage`, and first-boot
  storage resizing.
* ConnMan wired DHCP, a default route, and working OpenSSH/SFTP.
* Rejection of invalid SSH and UART passwords, authenticated UART login, and
  console restart after logout.
* A changed root password, SSH host key, SFTP data, and automatic console startup
  surviving poweroff and a fresh QEMU process; the old password was rejected.
* Successful module loading, including the image's `pkcs8_key_parser` module,
  and successful `add-entropy.service` startup.
* Native `strace` observing a successful `write(2)` and native `gdb` stopping an
  inferior, reading its PC, and continuing it to normal exit.

After the baud correction, the rebuilt image passed its SHA-256 check and its
shipped DTB passed `dt-validate` against the kernel bindings. A QEMU boot of the
new image exercised authenticated UART login and console restart after logout.
Native Python `termios.tcgetattr` observed both input and output at `B921600`
on each login, with `sc2-console.service` active. This checks the shipped login
service, not physical Meson UART timing.

A subsequent QEMU boot loaded the final `SYSTEM` and ran its actual boot-update
hook against an old 115200 `uEnv.ini`. It migrated the console to 921600 without
changing partition UUIDs or other arguments, then remounted `/flash` read-only.

Headless mode still includes Python3 because LE's entropy services require it;
this dependency is explicit rather than inherited from Kodi. Without kernel IP
autoconfiguration, initramfs creates an empty resolver file instead of reading
the absent `/proc/net/pnp`; ConnMan configures the live network later.

These checks do not execute the vendor U-Boot script or any SC2 peripheral.
The initial HK1 UART log reached Linux 6.18.57, four CPUs, and `ttyAML0`
registration, then became unreadable at console handover with the old 115200
setting. Complete HK1 boot, SD/USB, Ethernet PHY, and memory-reservation
compatibility still require hardware observation after the baud correction.
HDMI and multimedia remain disabled, not validated or repaired by the QEMU results.
