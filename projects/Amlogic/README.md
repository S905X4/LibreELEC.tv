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
recipe. It selects Mesa's Panfrost GLES path and DRM/GBM render diagnostics,
not Kodi, skins, the settings addon, FFmpeg, PulseAudio or PipeWire. The
independent `AMLGX` device is unchanged.
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

### Kernel source and separate review

The sibling `linux/` repository is the kernel development target, not one of
the donor trees under `reference-kernels/`. Its `hk1-sc2-6.18` branch starts at
official stable tag `v6.18.57`, commit
`2ff600d29ee478d0a8d46ef8a58afb5e038fe692`, from
`https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git`.
The first five commits preserve the initial peripheral baseline. Nine subsequent
commits add native IR, the status LED, FD6551 panel, audio clocks/arbitration,
HDMI output-divider readback, SC2 DRM/HDMI, digital ASoC playback and the panel
binding correction. A fifteenth commit corrects the SC2 HDMI DWC/TOP resource
order after tracing the vendor driver's indexed accesses; the sixteenth adds
the reusable default CMA pool required for native scanout allocations. The seventeenth
corrects protected SC2 HDMI register access and initializes inactive HDCP2.2 control.
The generated patches in `devices/SC2/patches/linux/` are LibreELEC's integration copy. Review
kernel changes separately from LE's recipe, configuration, image and boot-script
changes. Donor trees are not development targets or proof of upstream support.

The LE review branch is also named `hk1-sc2-6.18`, based on official LE
`12.2.1` (`6209f5a3b067c45fbf527286dd84ee483294b46a`). Review and commit
each repository independently; kernel commits do not replace LE integration
commits, and neither local branch is an upstream acceptance claim.

The initial source series migrates the existing patch payload into reviewable
commits; the later native-peripheral series has a combined build and userspace
smoke run, not a physical output acceptance result. Preserve donor attribution
and licenses when reviewing or exporting it. Local commits
use `hiper25 <whydonit@gmail.com>` and distinct `Change-Id` trailers without
adding `Signed-off-by`. This local rule does **not** satisfy Linux upstream's
DCO sign-off requirement: these commits are local review material, not an
upstream-ready submission. Retain any existing donor sign-offs.

Export each kernel commit in oldest-first order from the workspace root:

```sh
git -C linux format-patch --stdout --no-signature --full-index --binary \
  -1 FULL_COMMIT_SHA > Work/sc2-kernel-series/LE_PATCH_FILENAME
```

Use the corresponding existing filename, in order:

1. `0001-amlogic-sc2-clock-controller.patch`
2. `0002-amlogic-sc2-pinctrl-gpio-reset.patch`
3. `0003-amlogic-sc2-usb-phys.patch`
4. `0004-amlogic-sc2-hk1-rbox-x4.patch`
5. `0005-uapi-vhost-types-include-stddef.patch`
6. `0006-meson-sc2-ir-receiver.patch`
7. `0007-meson-sc2-hk1-status-led.patch`
8. `0008-meson-sc2-fd6551-panel.patch`
9. `0009-meson-sc2-audio-clocks.patch`
10. `0010-meson-sc2-audio-arbiter.patch`
11. `0011-meson-sc2-hdmi-od-readback.patch`
12. `0012-meson-sc2-native-hdmi.patch`
13. `0013-meson-sc2-native-audio.patch`
14. `0014-fd6551-indicator-reg-schema.patch`
15. `0015-meson-sc2-hdmi-register-banks.patch`
16. `0016-meson-sc2-default-cma-pool.patch`
17. `0017-meson-sc2-hdmi-secure-registers.patch`
18. `0018-meson-sc2-panfrost.patch`
19. `0019-meson-sc2-cecb.patch`
20. `0020-bluetooth-h5-shared-lpo.patch`
21. `0021-hk1-rbox-x4-nec-keymap.patch`
22. `0022-meson-sc2-secure-dvfs-clocks.patch`
23. `0023-meson-sc2-pwm-waveforms.patch`
24. `0024-pwm-regulator-boot-selector.patch`
25. `0025-opp-preserve-regulator-callback.patch`
26. `0026-meson-sc2-calibrated-thermal.patch`
27. `0027-meson-sc2-cpufreq-sequencing.patch`
28. `0028-meson-sc2-mode-aware-usb3-phy.patch`
29. `0029-meson-sc2-board-peripherals.patch`
30. `0030-meson-sc2-thermal-tuple-schema.patch`
31. `0031-meson-sc2-cecb-clock-ids.patch`
32. `0032-meson-sc2-inverted-pwm-roundtrip.patch`
33. `0033-hk1-rbox-x4-rc-map-binding.patch`
34. `0034-meson-sc2-inverted-duty-rounding.patch`
35. `0035-meson-sc2-secure-pll-output.patch`
36. `0036-meson-sc2-sdio-functional-clock.patch`

`Work/sc2-kernel-series.json` records the initial five exports;
`Work/sc2-peripheral-verification.json` records the later source commits,
exported patch hashes, artifact identities and exercised verification. Create the output
directory before exporting; replace `FULL_COMMIT_SHA` and `LE_PATCH_FILENAME`
with the corresponding recorded values. Copy each complete generated file to
`LibreELEC.tv/projects/Amlogic/devices/SC2/patches/linux/`; do not hand-edit its
diff or regenerate it from a donor branch. Git may normalize diff context and
hunk layout when exporting; the applied source payload must stay unchanged.

#### Stock-guided SDIO functional clock correction

Offline REA disassembly of stock Linux 5.4.125 confirms that the controller's
local divider consumes the external functional gate (`mux1`), while the
external mux (`mux0`) selects XTAL for identification. The stock Image SHA-256
is `8917da481e7db4cd172b85006f6a3496cf9f0a2787d334ecf4e9c129849db52d`.
Recovered kallsyms supply address labels, not original source or DWARF.
The exact-byte analysis carrier and mapping are in `Work/sc2-stock-native/`;
the principal REA observation is
`ev_175d9f627e236c6c415bd4a00f74998f46e191ef2aa1f519eb463811a7b31228`.

Patch 0036 connects SDIO A's `clkin0` to `CLKID_SD_EMMC_A_CLK`, rather than
direct XTAL, and assigns its external mux to XTAL and its functional clock
to 24 MHz. This makes CCF own the functional gate and removes dependence on
its inherited mux/divider settings. CE, Khadas and hkallweit model the same
chain. No private rescan suppression, PWM alternation API, raw register clear,
async-policy change, voltage change or capability increase is imported.
The downstream raw resource `0xfe000164` is not silently equated with the
clock provider's SD_EMMC control register at `0xfe00016c`.

The production-config DTB builds. Actual `fdtget` reads distinguish the old
direct-XTAL contract from the corrected functional-clock contract, verify
24 MHz defaults, and retain B/C clocks plus A's reset, IRQ and board policy.
Complete DTB schema validation returns zero with no diagnostics, using the
existing complete repository/core schema cache because bindings are unchanged.
All 36 exported kernel patches also replay in order against their exact pinned
upstream preimages in an isolated temporary repository; all 86 touched result
files match the development kernel byte-for-byte. Replay records are in
`Work/sc2-stock-native/series-replay.json`; temporary preimages were removed.
The resulting 21,595-byte DTB SHA-256 is
`3faeb4823fbee9d861024aeecb158a8e6a1d47a4b50e61c80f7e2e6dedb76f25`.
These are compiled-DT checks, not an SC2 boot or measured gate/LPO waveform.
The independent pre-userspace stall remains unverified; no new complete image
was built, installed or booted, and the owner-operated diagnostic hold remains.

### Consolidated peripheral candidate

This candidate adds native Panfrost matching for SC2 Mali-G31, CECB with its
external 32 kHz clock, the stock map_0 NEC keymap, RTL8821CS SDIO and H5 serdev,
calibrated CPU/DDR thermal zones, shared CPU/DSU DVFS and real PWM_J voltage
control. The front-panel GPIO translation is corrected to GPIOA_14 DAT and
GPIOA_15 CLK; GPIOB_0/1 are eMMC data lines. The TF path gains its stock drive,
detect and gated-clock pin states. eMMC C is enabled conservatively at 52 MHz
without HS200/HS400 or invented supplies. The combo PHY selects its consumer's
lane before enabling the reference PLL; USB3 is a candidate capability, not a
physical pass. Existing USB2 boot quirks and the 921600 console stay unchanged.

CPU policy uses all thirteen exact stock OPPs, 100..2004 MHz and 779..1019 mV,
not CE's generic multi-table/fuse policies. BSP's initial 667000 kHz floor is
not the table's 666000 kHz OPP; this candidate uses normal cpufreq-dt policy.
The provider temporarily parks CPU dynamic changes on the existing 24 MHz
crystal branch, below every OPP, rather than transiently using 1 GHz at the
low OPPs' 779 mV. Secure mux/divider rates are read from hardware, not merely
the requested CCF topology. CPU/DSU errors latch further voltage/clock changes
off and retain the old/raised voltage. GP1's inherited output/divider topology
may still prevent one of the requested 1.2/1.5 GHz DSU rates; exact readback
must pass before accepting any corresponding CPU transition.

PWM handover identifies a unique exact hardware-quantized voltage-table
selector **before** probe-time adjustment or enable. Disabled, unrecognized,
ambiguous, blinking or secondary waveforms fail without rewriting the rail.
The actual PWM_J parent is inherited; CPU policy rejects mutable video-PLL
ancestry. No guessed GPIO, voltage tolerance or nearest selector is used.
Thermal trim validity is mandatory; invalid trim is not replaced by zero.
Polling, CPU cooling and 105 C critical shutdown are described; the stock
autonomous 110 C reboot is not newly programmed. Do not run unmonitored stress.

Wireless uses existing rtw88 and native H5/btrtl, not an out-of-tree attach
daemon. UART_E at fe080000 is serial1; console UART_B is unchanged. PWM_E's
fixed FCLK_DIV3 source supplies a shared nominal 32768 Hz clock to WiFi and
Bluetooth. Actual waveform/tolerance, ROM identity, RF and paired HID input
are physical acceptance items. SDIO stays at 50 MHz/3.3 V without invented
1.8 V switching or OOB wake support. The three Realtek firmware files and
their upstream redistribution notice are included unmodified.

Kodi and HEVC/AV1 decoding remain absent. Mesa Panfrost, kmscube and glmark2
provide native DRM/GBM rendering diagnostics; module binding alone is not
rendered output. HEVC and AV1 each still lack the implemented
controls-authoritative Request/context contract, and the selected FFmpeg has
no AV1 Request backend. Firmware bytes have been recovered, but no applicable
firmware/TA redistribution grant was identified; those blobs are not shipped.
Source and protocol evidence is in workspace `Work/sc2-batch-video.json` and
`Work/sc2-video-evidence-bundle.json`. Analog AVDD/jack wiring remains unknown.

#### Built candidate and automatic verification

Linux `5ba418afc34643f279dc17870f0e927f7c807f16` and LE build commit
`971df5cc5359f587090684210f86c71c26c66f00` produced
`LibreELEC-SC2.aarch64-12.2-devel-20261020012723-971df5c` in `target/`.
The last fully verified installed baseline was `20261019224356-c17c620`.
The owner subsequently authorized a standard online update and reboot.
SSH did not recover. The owner's later UART log confirms that the new
USB-loaded kernel entered Linux and failed during initial CPU/DSU scaling;
the installed SYSTEM hash is still unverified. Later documentation commits
do not change the identity of the built candidate.

The image SHA-256 is
`616689a5e918479f792a5e2c5b0e6355ba7fe58f997d44ef4984e7e09944ca3f`;
the standard update TAR SHA-256 is
`027fd7ff990418d51d7c614fb582fb840f04075af3b5b9fa883e5e6d4f5edadd`.
`Work/sc2-batch-acceptance.json` indexes complete build/runtime evidence,
including retained failed attempts rather than only the final success.

The exact packaged board DTB passed full-schema validation without
diagnostics and all 28 semantic checks. Host execution of extracted
production functions passed 738 PWM assertions, 510650 thermal arithmetic
assertions, and 4932 CPU/DSU sequencing cases with 195629 assertions. The
models cover mathematical boundaries and injected failures, not Linux
framework concurrency, actual rail voltage, secure firmware or calibration.

The shipped SYSTEM and extracted embedded initramfs booted on a private
QEMU disk with a separate current-source diagnostic kernel: real first-boot
resize/reboot, persistent storage, authenticated SSH/SFTP with a checked
1 MiB roundtrip, native module loading, 256 MiB CMA, tool startup and ALSA
null PCM all passed. Persistent-mode system state reached `running` with
no failed units. QEMU lacks SC2 UART_B: a RAM-only udev rule mapped its real
PL011 `ttyAMA0` to `ttyAML0`; the unmodified production console service
then ran normally. Standard outbound user-mode NAT allowed real time-sync;
SSH forwarding stayed loopback-only. No production service was masked and
neither fixture changes the shipped SYSTEM. This is not a physical UART,
Panfrost rendering or audible-audio pass.

The separate `toram live` entry also booted that SYSTEM: the root loop used
`/dev/SYSTEM`, `/storage` was tmpfs, no swap was active, and `/flash` was
explicitly unmounted. SSH/ConnMan and a matching RAM SYSTEM hash remained
available with neither virtual card partition mounted. This proves the
VM RAM/live path, not that physical boot media may be removed without the
same mount, swap and open-file checks on the box.

Checkpatch is **not fully clean**: its 17 errors all reject the locally
required Change-Id footer. Ten warnings retain binding/implementation
split advice, MAINTAINERS advice and the thermal minimum-settling
`msleep(10)` timing-slack advisory. No warnings were hidden; the explicit
local no-signoff policy used `--no-signoff`. These commits are not claimed
to satisfy Linux upstream submission/DCO requirements. Decoding the DTB
with the system dtc also retains the existing Meson pinctrl empty-ranges
warning; final full-DTB schema validation itself has no diagnostics.

#### Authorized online update: failed candidate boot

Before staging, authenticated SSH confirmed the `c17c620` baseline,
`running` state, an empty update directory and the USB SD Card Reader's
`/dev/sda1` read-only `/flash` plus `/dev/sda2` `/storage`. The complete boot
partition was backed up on the box and downloaded to
`Work/sc2-pre-batch-20261020012723-971df5c.tar`: 69185024 bytes, SHA-256
`9a762a96e8d7ff7b8abb919d9c45f79b49f7ad2dbd173fd77e882c3537ba0cfe`.
It contains the old KERNEL, SYSTEM, DTB, startup script and original UUIDs.

The 83742720-byte candidate update TAR was uploaded outside `.update`,
checked against the TAR hash above, synced and atomically moved into the
empty `/storage/.update/` directory. `/flash` remained read-only during
staging. After explicitly notifying the owner, `sync && systemctl reboot`
returned zero. This proves staging and the accepted reboot request, **not**
completion of the updater or successful candidate boot.

At 513.58 seconds after the request, SSH to the old `10.0.0.151` address
still returned `No route to host`; its ARP neighbor was `FAILED`. mDNS
returned no reply. Bounded discovery of the directly connected `/24`,
including TCP22 independently of ICMP replies, found no SSH server with
the board's previously trusted Ed25519 host key. No login to unrelated
hosts was attempted. The later owner UART log establishes the CPUFreq boot
failure below. No post-boot peripheral, voltage or stress tests passed.

`Work/sc2-batch-deployment.json` retains the transaction and recovery
observations. The box-side backup is
`/storage/sc2-pre-batch-20261020012723-971df5c.tar`; neither that file nor a
local backup is an automatic rollback. At that stage recovery required the
owner-operated 921600-baud console or removable boot medium. No further
remote reboot, guessed low-level write or eMMC flash was attempted during
the failed-update transaction. The later completed recovery is recorded
below; vendor U-Boot's automatic environment saves remain outside the
no-explicit-eMMC-write claim.

#### Confirmed CPU/DSU boot regression and one-time recovery

The owner subsequently supplied a complete physical UART log. U-Boot read
the 25346624-byte KERNEL and 25883-byte DTB from USB, verified the uImage
checksum and entered Linux 6.18.57 built at 00:47:07. At 0.941883 seconds,
`CPU/DSU transition failed (-5); retaining voltage` preceded the initial
frequency-correction `BUG_ON(ret)` in `drivers/cpufreq/cpufreq.c:1529`.
The failure is therefore not merely a missing DHCP lease or an unbootable
vendor bootloader.
The original UART log does not identify the exact failed clock call or
measure the rail. The PLL ABI defect was subsequently reproduced below.
The separate eMMC voltage-negotiation `-22` is also not evidence that the
vendor partition table was corrupted, nor the identified cause of this BUG.

Do not redeploy this candidate unchanged, suppress the CPUFreq core BUG,
pretend that a failed transition succeeded, or add a guessed OPP. First
recover the last physically verified `c17c620` image. The delivered config
has KALLSYMS and its packaged System.map contains `sc2_cpufreq_init`.
SC2 is also excluded from generic CPUFreq platform-device creation. At
the owner-operated 921600-baud U-Boot prompt, this one-time rescue skips
the SC2 initializer and CPUFreq registration without persistent changes:

```text
usb start
fatload usb 0:1 0x21000000 uEnv.ini
env import -t 0x21000000 ${filesize} bootargs
setenv bootargs "${bootargs} initcall_blacklist=sc2_cpufreq_init cpufreq.off=1 toram"
fatload usb 0:1 0x12000000 KERNEL
fatload usb 0:1 0x20000000 amlogic/meson-sc2-hk1-rbox-x4.dtb
fdt addr 0x20000000
fdt resize 0x10000
setenv fdt_high ffffffffffffffff
bootm 0x12000000 - 0x20000000
```

Stop on any USB/read/import/FDT failure; do not run `bootm` after an error.
Do not `saveenv` or rerun the autoscript, which reimports the original
bootargs. `toram` copies SYSTEM to `/dev/SYSTEM`; it does not imply `live`,
and the imported original disk UUID preserves persistent `/storage`.
The owner confirmed that the blacklist took effect, but this candidate
still stopped after a USB discovery message without a userspace prompt.
That last message does not establish a USB root cause or prove that
`toram` ran. The matched accepted KERNEL/DTB rescue was used instead.

After authenticated SSH recovery, require the root loop backing file to
be `/dev/SYSTEM` and rediscover the USB provenance of `/flash` and `/storage`
before writing. Never overwrite a SYSTEM still backing the mounted root.
The five rollback payloads have been extracted and SHA-256 checked in
`Work/sc2-recovery-c17c620/`: KERNEL, SYSTEM, the board DTB, s905_autoscript
and the original uEnv.ini. KERNEL and SYSTEM also match the retained
known-good release artifacts byte-for-byte by SHA-256. The old backup's
MD5 sidecars are stale and must not be used as verification authority.
Stage and verify every payload before promotion, preserve the original
UUIDs, keep any of this transaction's remaining candidate TAR out of
`.update`, sync and restore `/flash` read-only before a notified reboot.
The owner subsequently restored and verified these five payloads from
the matched known-good RAM rescue. Authenticated SSH now confirms normal
`c17c620` operation, `running`, no failed units, and installed payload hashes
matching that recovery authority. `/flash` is `/dev/sda1`, mounted read-only;
`/storage` is `/dev/sda2`. Earlier recovery events remain in
`Work/sc2-cpufreq-recovery.json`; current observations and PLL checks are
indexed by `Work/sc2-pll-fixed-verification.json`.

#### SC2 secure PLL output ABI correction

The board's firmware accepts the final PLL output frequency, not the
internal DCO frequency. With CPU safely on the independent dynamic path,
the old 3216 MHz DCO request produced a 3072 MHz DCO divided by 128;
requesting the original 3024 MHz DCO did not restore it. A 1512 MHz output
request restored the exact 3024 MHz DCO and divide-by-two configuration.
The wrong boundary is therefore a physical observation, not a host model.

SYS and GP1 now use SC2-specific output operations: one firmware request
programs M, N and OD together, then checks lock and the actual output rate.
Their published DCO clocks remain read-only; CCF must not independently
program a DCO and divider or propagate an output request to the DCO.
Output recalculation reads the DCO hardware rather than its stale CCF
cache. BSP output tables constrain rounding and programming. The generic
secure-DCO writer is removed; ordinary Meson PLLs are unchanged. This does
not add an OPP, suppress the CPUFreq BUG, or bypass transition failures.

An isolated diagnostic module ran the extracted production operations on
the real board, with both CPU and DSU parked at 996999996 Hz. All sixteen
SYS rates from 1008 through 2004 MHz and both GP1 rates (1200/1500 MHz)
matched their exact CCF readbacks. Both consumers stayed parked during
each request; no CPU voltage operation or active high-frequency CPU test
was performed. Restoration returned CPU to 1512 MHz, DSU to 1200 MHz and
the spare branch to 24 MHz. SYS/GP1 CTRL0 returned to `d001047e`/`d00204c8`;
CPU, DSU and sync controls returned to `a4010800`/`10003`/`8000122`.
The temporary module was unloaded and its box-side payload removed.

The restored baseline remains `running` with no failed units, unchanged
boot payload hashes and read-only `/flash`. Kernel taint is 4096 from the
out-of-tree diagnostics; unloading them does not clear that bit. Clock
objects built with the production config; `W=1` reports the existing
duplicate `CLKID_VID_PLL` initializer. Strict patch checks retain the
atomic `udelay(10)` advice: the same path serves `.enable()` and cannot
sleep. Raw before/failure/restoration and corrected live logs are indexed
in `Work/sc2-pll-fixed-verification.json`.

This verifies the firmware boundary and parked-consumer output operations,
not full OPP/regulator/thermal policy or a newly deployed image. The
separate startup stall with CPUFreq skipped is still unresolved. The next
notified, owner-observed diagnostic boot must capture that path before any
claim of full candidate recovery; no new image was deployed or rebooted
as part of this PLL probe.

#### Opt-in early-shell diagnostic after the PLL fix

The owner keeps the 921600-baud TTL console on a different host. Do not
probe this workstation for it. Notify the owner before a remote reboot;
the owner must interrupt U-Boot autoboot to select this diagnostic.

The corrected production-config build from Linux `c9c7080efcd2` has been
wrapped as an uncompressed arm64 uImage with load/entry `0x02000000`, and
staged with its matching board DTB in `/flash/sc2-diag-c9c7080/`. Only that
new directory was added: normal KERNEL, SYSTEM, board DTB, s905_autoscript
and uEnv.ini still match the accepted `c17c620` hashes. `/flash` is again
read-only and `.update` is empty. The existing storage backup TAR also
matches SHA-256 `9a762a96e8d7ff7b8abb919d9c45f79b49f7ad2dbd173fd77e882c3537ba0cfe`.
Staging is verified; no physical diagnostic boot result is claimed here.

| Diagnostic file | Bytes | Whole-file CRC32 |
| --- | ---: | --- |
| `sc2-diag-c9c7080/KERNEL` | 25346624 | `2aa4829c` |
| `sc2-diag-c9c7080/board.dtb` | 21527 | `b5f435cd` |

At the owner-operated U-Boot prompt, run the following in order. Stop at
any load/FDT error, unexpected byte count or CRC mismatch; do not boot
stale buffers after an error. Check each CRC before the next load/step.
These are RAM-only environment changes: do not `saveenv`, run the normal
autoscript again, or issue MMC/firmware writes.

```text
usb start
fatload usb 0:1 0x12000000 sc2-diag-c9c7080/KERNEL
crc32 0x12000000 0x182c240
fatload usb 0:1 0x20000000 sc2-diag-c9c7080/board.dtb
crc32 0x20000000 0x5417
fdt addr 0x20000000
fdt resize 0x10000
setenv bootargs "console=ttyAML0,921600n8 earlycon ignore_loglevel loglevel=8 initcall_debug log_buf_len=4M initcall_blacklist=sc2_cpufreq_init cpufreq.off=1 rdinit=/usr/bin/sh"
setenv fdt_high ffffffffffffffff
bootm 0x12000000 - 0x20000000
```

This requests the embedded early shell, not `/init` or the installed
SYSTEM. No disk UUIDs, SSH/userland options or `toram` are passed as shell
arguments. CPUFreq stays disabled; unused-clock and domain cleanup are
not bypassed to hide the stall. Capture the full log from U-Boot until
the shell, or through at least 60 seconds after the last output. A job-
control warning at the `#` prompt is expected. Do not exit the PID1 shell
or start `/init` before the log has been reviewed. If the boot stalls,
reset/power-cycle without manually selecting this directory to use the
unchanged baseline again. This is a recovery path, not an automatic
failure detector; vendor U-Boot may still save its own environment.

Both uImage CRCs, exact Image payload, DTB length, staging/relocation RAM
windows and uploaded/readback SHA-256 passed. The exact embedded initramfs
also reached its real early shell under the existing QEMU transport kernel;
RAM-only proc/sys mounts and `/proc/cmdline` reads worked without disks or
SYSTEM. The first harness attempt exposed a missing BusyBox echo applet
and absent mountpoint directories; its log is retained. Shell `printf`
plus actual BusyBox mkdir/mount passed without changing the boot payload.
This does not verify SC2 peripheral initialization or reproduce its stall.
Commands, hashes and raw logs are indexed by `Work/sc2-diag-boot.json`.

#### Copyable USB test for the SDIO clock correction

`Work/sc2-test-0e17edd.zip` is a 10,456,378-byte manual boot package, not a
complete image or standard update TAR. Its SHA-256 is
`1307416909b2f5ae172d4446bc4a6253e1f45c971373aed9afe95315f6d79f79`.
It contains the existing verified `c9c7080` diagnostic KERNEL and the corrected
`0e17edd` board DTB, plus `README.txt`, `BOOT.txt` and `SHA256SUMS`. The only
kernel source change between those commits is the DT source; the uImage's
payload is byte-identical to the already-built production-config Image.
Do not interpret its unchanged kernel name as an old DTB or a fresh image build.

With the box powered off, copy the complete `sc2-test-0e17edd` directory from
the ZIP to the existing USB FAT boot partition root, alongside the normal
KERNEL and SYSTEM. Do not format the disk or overwrite the root KERNEL,
SYSTEM, board DTB, uEnv.ini, s905_autoscript or old diagnostic directory.
This package has only been prepared on the host; it has not been uploaded
to the box. Safely eject the USB, reconnect it, record the existing 921600
UART on the owner's other host and interrupt autoboot with space.

Run the package's `BOOT.txt` one line at a time at the U-Boot prompt, stopping
on any error or mismatch. It uses the previously established `usb 0:1`:

```text
usb start
fatload usb 0:1 0x12000000 sc2-test-0e17edd/KERNEL
crc32 0x12000000 0x182c240
fatload usb 0:1 0x20000000 sc2-test-0e17edd/board.dtb
crc32 0x20000000 0x545b
fdt addr 0x20000000
fdt print /soc/mmc@fe088000 clocks
fdt print /soc/mmc@fe088000 assigned-clock-rates
fdt resize 0x10000
setenv bootargs "console=ttyAML0,921600n8 earlycon ignore_loglevel loglevel=8 initcall_debug log_buf_len=4M initcall_blacklist=sc2_cpufreq_init cpufreq.off=1 rdinit=/usr/bin/sh async.dyndbg=+pmt dd.dyndbg=+pmt meson_gx_mmc.dyndbg=+pmt mmc_core.dyndbg=+pmt"
setenv fdt_high ffffffffffffffff
bootm 0x12000000 - 0x20000000
```

KERNEL must load 25,346,624 bytes and report CRC32 `2aa4829c`. The new DTB
must load 21,595 bytes and report CRC32 `f9458ddc`; the old `0x5417` DTB
length/CRC must not be reused. Verify CRC before modifying FDT in RAM.
The clocks cells are `3 111 3 dc 3 5` in hex, and assigned rates are
`0 016e3600` in hex (`0 24000000` decimal). Do not manually save the
environment, run an installer, issue MMC writes or use the `.update` path.
Automatic vendor U-Boot environment writes remain outside this guarantee.

The requested result is the serial early-shell `#` prompt, not HDMI output,
SSH or a normal LibreELEC login. CPUFreq stays off. Do not exit PID1 or run
`/init`; send the complete UART log first. If it stalls, retain at least
60 seconds after the last output. BREAK/SysRq is not required: the command
enables the previously exercised automatic async/driver/MMC debug selectors.
Power-cycle without selecting this directory to use the untouched defaults;
this is a recovery path, not an automatic rollback monitor.

The real ZIP was extracted, copied by mtools to a disposable 64 MiB FAT image,
and all five files read back byte-identically. `dumpimage` accepts the uImage;
`fdtget` reads the corrected clock contract. Header/data CRC, DTB length and
RAM-window checks pass. The temporary FAT image was removed; no physical
media was opened. Evidence: `Work/sc2-test-0e17edd-package.json`.

#### Accepted normal userspace and explicit MMC voltage capabilities

The owner's complete UART transcript confirms that `sc2-userspace-0e17edd`
reached native `/init`, systemd and authenticated 921600-baud serial login:
system state `running`, no failed units, Ethernet `10.0.0.151/24`, read-only
USB `/flash` and tmpfs `/storage`. This supersedes that package's historical
physical-boot-pending status; it is not a repeated assistant boot. Subsequent
authenticated, read-only SSH matched the KERNEL, DTB and `/dev/SYSTEM`
SHA-256 values to the package. CPUFreq remains explicitly disabled. Evidence:
`Work/sc2-userspace-0e17edd-owner-observation.json` and
`Work/sc2-userspace-live-readonly.log`.

That boot still rejected eMMC and Wi-Fi SDIO with `no support for card's
volts` / `-22`. The runtime DT has no MMC supply or voltage declarations.
The mainline Meson host obtained no regulator-derived OCR mask and supplied
no alternative, so `mmc_select_voltage()` rejected the responding cards.
The CE 5.4.210 reference driver instead initializes `ocr_avail = 0x200080`;
this source is not evidence of a measured rail or of the stock 5.4.125
binary's exact implementation.

Linux commits `fb49ce891214`, `0049fc975bbd` and `b5a3ca554b71`, exported as
SC2 patches 0037--0039, describe and parse the existing `voltage-ranges`
convention only when no regulator-derived OCR mask exists. The three board
hosts explicitly declare nominal 3300 mV card-power capability. Standard
conversion includes both boundary bins, producing `0x00300000`, not the
reference driver's mask; no 1.8 V capability is advertised. This does not
create a regulator, guess a GPIO, control bus IO voltage, increase existing
50/50/52 MHz limits or enable HS200/HS400. Physical supply voltage remains
unmeasured. Existing regulator-backed boards retain precedence, and boards
without either declaration retain their previous behavior.

The manual candidate is `Work/sc2-mmc-voltage/sc2-mmc-b5a3ca5.zip`,
78,101,478 bytes, SHA-256
`d02ee260d6f5dcfe2d90525969089c759c0183befc208671516efbb7e8595c3a`.
It contains an independent `sc2-mmc-b5a3ca5/` directory with KERNEL, board.dtb,
the unchanged previously authenticated SYSTEM, checksums and Chinese
operation/rollback instructions. It is not a complete disk image or update
TAR. With the box properly powered off, copy that whole directory to the
original USB FAT root without changing its defaults or formatting it.
At the next owner-operated U-Boot session, execute its `BOOT.txt` line by
line. KERNEL must load 25,346,624 bytes (`0x182c240`), CRC32 `8beb121a`;
the DTB must load 21,670 bytes (`0x54a6`), CRC32 `e22e55b0`. Stop on any
mismatch; do not reuse the old DTB length or either old CRC.

The commands preserve the original USB boot UUID `1910-5516`, UART 921600,
normal `/init`, `toram live`, `nofsck`, CPUFreq hold and full three-argument
`bootm`. No extra TF insertion, cable reconnection or peripheral action is
required. Capture UART from `bootm` through login; check eMMC block-device
and SDIO-function enumeration and preserve any subsequent timeout, tuning,
CRC or firmware error. Do not mount, format, write or stress-test internal
eMMC. Enumeration does not prove Wi-Fi association, storage writes or DVFS.
Do not run `saveenv`, an installer or `.update`; vendor automatic STORAGE
environment saves remain outside a whole-boot zero-write guarantee.
The candidate has not been uploaded or physically booted. No default
boot file, physical media or running MMC state was changed, and no reboot
was requested by the assistant. Not selecting the directory retains the
accepted default boot files.

Production `Image dtbs modules` compilation, binding validation and full
DT schema validation passed. A throwaway C smoke executed the actual
production voltage conversion with values from the delivered DTB, including
boundary/invalid ranges and card-OCR intersection. All 66 shipped modules'
4,084 imported symbol CRCs match the new kernel. The initramfs has the same
54 paths, contents, modes, owners and link counts; 30 regenerated mtimes
change its compressed bytes. The exported three-patch series was actually
applied to its pre-series source and reproduced all three production files.
uImage header/data CRCs, exact Image payload and RAM windows passed; the
actual ZIP was extracted, copied through a private FAT image and all six
files read back byte-identically. That image was removed; no physical USB
was opened. This is host proof, not HK1 MMC acceptance. Strict checkpatch
reports six local-policy conflicts: three required Change-Id trailers and
three intentionally absent Signed-off-by trailers; no warnings or checks.
The series is not upstream-submission-ready. Evidence:
`Work/sc2-mmc-voltage/verification.json`.



#### One combined hardware acceptance round

Commit the complete source and LE integration before the combined image build.
Exercise that exact candidate as a whole; do not request per-feature cable,
card or remote operations while adapting its source. Record kernel/image/DTB
identity, calibration, policy, clocks, deferred probes, firmware, CEC and RF
enumeration first. Only with working calibrated thermal readout and understood
PWM_J handover may a bounded, temperature-monitored CPU transition run proceed.
Reject failed transitions rather than lowering voltage or disguising errors.
Then exercise existing HDMI/48 kHz stereo DMA and finite page flips, GPIO LED,
panel, actual IR keys, WiFi scanning and Bluetooth input in the same session.
Record receiver-visible/audio results separately from counters and registration.
Exercise actual Panfrost rendering with the installed kmscube/glmark2 DRM/GBM
tools, verify the reported renderer is Mali rather than a software fallback,
and observe displayed output. TCL direct cold-boot picture persistence is a
separate physical scenario; CEC registration alone does not prove it.

The SC2-only early udev policy sets force_ro=1 for eMMC user/boot/GP disks by
MMC ancestor type, never by mmcblk numbering alone. Before inventory, settle
udev and verify every touched disk's `/sys/class/block/DEVICE/ro` is `1`. Only
sysfs identity and O_RDONLY partition-table reads are allowed: no automount, fsck,
installer, mmc-utils, RPMB access, bootpart/bootbus/partconf or power-on write
protection commands. force_ro is block-layer protection, not raw-ioctl-proof
hardware write protection; privileged MMC_IOC_CMD bypasses it. Enumeration has
a pre-udev interval and normal MMC initialization may change volatile state.
Stock U-Boot may itself save its environment; do not claim zero eMMC commands.

Keep the current USB2 root card connected. Native TF transfer testing needs a
separate card, or a verified RAM/live boot using the separate uEnv.live.ini:

```text
setenv sc2_live 1
usb start
fatload usb 0:1 0x22000000 s905_autoscript
autoscr 0x22000000
```

Do not save the environment. Normal uEnv.ini UUIDs are preserved; the separate
entry uses toram live and no disk= argument. Before moving any media, verify
SYSTEM was copied to RAM, storage is tmpfs, no backing mount/swap/open file
depends on the card, and explicitly unmount /flash. Merely reaching login
does not free the card. U-Boot's mmc numbering is not Linux's MMC alias order.
Use a separate, expendable USB3 device for SuperSpeed enumeration/transfers;
never hot-unplug the boot/root device to test a port.


### SC2 protected HDMI controls

Linux `f36ff4f96555de48f8c0e4f7e40dc54b22384787` routes protected DWC
clock-disable, HDCP configuration and HDCP2.2 control registers through
firmware SMC `0x82000018`/`0x82000019`, using encoded secure selectors.
Ordinary DWC registers and non-SC2 variants retain their existing access.
The unencrypted bridge initializes HDCP2.2 control to `0x02`, matching
the SC2 CoreELEC reference driver's initialization; this does not add HDCP authentication.

The board reproduced the access defect: HDCPCFG1 remained `0x01` despite
the generic bridge requesting `0x03`. A reversible SMC probe wrote and read
`0x03`; a subsequent byte-MMIO attempt to write `0x01` still read `0x03`.
The inactive HDCP2.2 value was independently read back through SMC.
Source, probe and direct-write-control logs are retained under
`Work/sc2-hdmi-blank-secure-*.log` in the workspace.

The stock-declared SC2 frequency counter measured ENCP, HDMI pixel, TMDS
and frontend clocks at approximately 148.5 MHz, VPU at 666.6 MHz and VAPB
at 250 MHz. Its programmable configuration was restored after measurement;
the counter's completion-status bit remained set. Zero-rate CCF summaries
therefore do not prove stopped video clocks and are not the target of this fix.

An encoder hardware-color-bar trial explicitly enabled `VENC_VIDEO_TST_EN`
and cleared `ENCP_VIDEO_MODE_ADV[3]`, unlike a framebuffer SMPTE test.
Register readback proves that selection, not a visible sink image. The
temporary probe and BIST are diagnostic-only, with rollback; physical
image quality, colors and direct-TCL cold-start behavior remain acceptance gates.

#### Production artifact and real-board verification

LE image-build commit `c17c6206475e2a6150c3ba47b89444e68eb32fe1` produced
`target/LibreELEC-SC2.aarch64-12.2-devel-20261019224356-c17c620`:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `.img.gz` | 55,112,142 | `d156755bd9ec0153f0fcefd01da8e849747be05443899c5945771cf2727459e9` |
| `.tar` | 69,754,880 | `fe209393a902cc0551a5005042e159c3fcbd9a91c3b7a28a971fccbe6fe9931b` |
| `.kernel` | 24,865,344 | `aeeafd54dce08f76b581061f519be9912bb757aa5adc4a6398884f7a630ed98c` |
| `.system` | 44,290,048 | `58d70536a914e0ed102cbeff4f3a75e9c0cdf097681b7e1490d7305d1a03be5b` |

Sidecar SHA-256, TAR payload MD5, uImage header/data CRC and embedded CMA
configuration checks passed. The uImage retains load/entry `0x02000000`;
the removable script's RAM input buffer remains `0x12000000`. KERNEL,
SYSTEM, DTB and removable script match between image and TAR. The three
changed source files also match the integrated kernel build byte for byte.
Checkpatch reports zero errors and warnings with only the user-required
local Gerrit Change-Id exception; this is not an upstream/DCO acceptance claim.

Before the notified standard update, running boot files were backed up as
`Work/sc2-pre-secure-LibreELEC-SC2.aarch64-12.2-devel-20261019224356-c17c620.tar`
(69,178,368 bytes, SHA-256
`fa2672282e6235de0a78a5438d636d4820170da7e4f208ebfef29336030174ec`).
The uploaded update hash matched the local TAR. The new production image
returned over SSH after 50.34 seconds, reports `systemctl is-system-running`
as `running`, has no failed units and kernel taint zero. Installed KERNEL,
SYSTEM and DTB hashes match the verified artifact. `uEnv.ini` retains SHA-256
`6f6102d520ccb1c4adae96d3cd4856dd8c7ccd9e53296a650b468a39f93f187c`,
and `/flash` is again read-only on `/dev/sda1`. No explicit eMMC or persistent
U-Boot environment write was requested; vendor automatic writes remain outside
this observation without the owner's UART log.

Without a diagnostic module, the native driver now reads `MC_CLKDIS=0x6c`,
`HDCPCFG1=0x03` and `HDCP22REG_CTRL=0x02`. These protected values persist
through a 12-second 1080p60 SMPTE/page-flip run (60.45 Hz initially, then
60.00 Hz); the original framebuffer `fb=44` and active CRTC return on exit.
CMA remains 256 MiB. No hardware BIST is left enabled.

The shared protected clock register was also exercised through real PCM,
not ALSA null: `hw:0,1`, S16_LE, 48 kHz, stereo, two four-second
open/play/stop cycles. The DT route is FRDDR_B OUT2 to TDMOUT_C IN1 to
I2S C to HDMI. DMA hardware pointers advanced from 31,104 to 79,360;
the newly requested FRDDR_B IRQ advanced from 5 to 13 during the first run.
`MC_CLKDIS` transitions `0x6c -> 0x64 -> 0x6c`, proving native audio gate
enable/disable through the protected callback. All five touched mixer controls
were restored to their observed values and the temporary WAV was removed.
This proves PCM/DMA/IRQ and gate lifecycle, not audible stereo at a receiver.

`Work/sc2-hdmi-secure-verification.json` and its referenced live-state,
page-flip and PCM logs retain the evidence. The owner reported “有东西了”
and supplied an image of eight full-height vertical color bars in the order
white, yellow, cyan, green, magenta, red, blue and black. The image's generator
and capture time are unknown; color bars alone do not establish ordinary
framebuffer content at the HDMI sink, and a capture-device placeholder has
not been excluded.

A later live inspection found encoder BIST disabled (`VENC_VIDEO_TST_EN=0`,
`ENCP_VIDEO_MODE_ADV=0x18`), active fbcon `fb=44`, and `fb0/blank=4`.
Unblanking to `0` and writing `HK1 RBOX X4 - HDMI FRAMEBUFFER` to active
`tty1` produced readable text and RGB labels in a direct `/dev/fb0` capture,
`Work/sc2-hdmi-framebuffer-marker.png`. This is software-buffer evidence,
not an HDMI sink capture. The marker log is
`Work/sc2-hdmi-framebuffer-marker.log`. The owner then replied “有的” and
supplied a capture image showing that distinct marker, readable console text
and distinguishable red/green/blue/white labels. This passes ordinary
framebuffer-to-HDMI output in the current capture setup, rather than only
software readback or unexplained color bars. It does not establish calibrated
color fidelity, audible receiver audio, direct-TCL cold start or display-setting
persistence; those acceptance gates remain open.

### Removable boot only

Write the resulting image to an SD card or USB drive, **not eMMC**. The image
contains `KERNEL`, `SYSTEM`, `uEnv.ini`, `s905_autoscript`, and the preselected
`amlogic/meson-sc2-hk1-rbox-x4.dtb`. There is no U-Boot/FIP binary, activation
script, `installtointernal`, environment installer, or raw bootloader write.
In particular, the AMLGX `aml_autoscript` that runs `defenv`/`saveenv` is not
included. eMMC is not an installation target; the candidate's enumeration and
block-layer read-only limits are described above.

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

The target enables native IR, GPIO LED, FD6551 auxdisplay, DRM framebuffer
console, optical SPDIF, DW-HDMI I2S, Panfrost, CECB, native wireless, calibrated
thermal zones and coordinated CPU DVFS. Mesa Panfrost and DRM/GBM render tools
are selected; Kodi and hardware video decoding remain absent. Built drivers
and device nodes are not physical acceptance.
The normal distro-wide options audit also includes desktop/container/media
requirements outside this target; do not enable unrelated stacks for its sake.
CPU DVFS and thermal safety under sustained load remain unverified until the
consolidated candidate is actually exercised; simulations are not stress tests.

### USB boot enumeration

With the console corrected, the owner log reaches both xHCI root hubs but no
external USB device or block device, then stops at `mount_flash`. A root hub
does not prove that the USB2 PHY can detect the drive; changing partition UUIDs
or extending mount retries does not address the observed failure.

The HK1 board DT now preserves the stock `snps,dis_enblslpm_quirk` and
`snps,dis-u2-freeclk-exists-quirk` properties. Linux 6.18.57 consumes both:
they clear `GUSB2PHYCFG.ENBLSLPM` and `U2_FREECLK_EXISTS`. The corrected DTB
passes kernel schema validation. A host-only register smoke exercised the
actual DWC3 setup routine on both modeled ports with properties decoded from
that DTB; it is not a physical USB test.

A later owner-supplied hardware log shows an external Prolific USB SD card
reader enumerating, its storage partitions mounting at `/flash` and `/storage`,
and the system reaching authenticated UART login at 921600 baud. USB2 boot
therefore has physical-board evidence; native SD-slot and USB3 operation do not.
Keep this working boot path unchanged while investigating other peripherals.

### Ethernet link observation

The owner's runtime queries identify an RTL8211F (`0x001cc916`) at PHY address
0 in RGMII mode, with autonegotiation enabled. Initially `ethtool` reported
`Link detected: no` and sysfs reported `carrier=0`. The owner then reported
that unplugging and reconnecting the Ethernet cable restored networking.
This establishes hotplug recovery, not reliable automatic link establishment
at cold boot or sustained traffic stability. A later read-only SSH observation
of the running Linux 6.18.57 system reports `1000Mb/s`, full duplex, and an
active Ethernet link; this is negotiated link speed, not measured throughput.
No PHY, MAC delay, EEE, Wake-on-LAN, or DHCP settings were changed in response.
If it recurs, retain the pre-replug boot/link observations; the root cause
is not established by the recovery alone.

A bounded SSH transfer on the already-running Linux 6.18.57 / LE
`e4a311be12` system moved 33,554,432 bytes in each direction. Upload `fsync`,
readback, and downloaded-data SHA-256 checks matched
`7d3499e568e8698bbe6d1fa2eec33bd2cd6e917517312f9049098583abed6099`.
The link stayed at 1000 Mb/s full duplex with carrier present and
`carrier_changes=3` throughout. RX/TX error, CRC, frame, length, and carrier
error counter deltas were zero; `rx_dropped` increased from 21 to 22, with
no established cause. `/flash` remained read-only and `/storage` remained
ext4 on USB2 storage. Evidence is retained locally in
`Work/sc2-live-smoke.json` at the workspace root.

This confirms a bounded functional encrypted transfer, not line-rate Ethernet,
physical USB throughput, long-term stability, or power-loss persistence.
No reboot, kernel/DTB deployment, or persistent configuration change was made;
temporary board test data was removed. This does not validate the newly
exported kernel commits by building or deploying them.

On the next normal cold boot, leave the same cable connected to the same
powered switch port. Check whether networking returns without touching it.
If not, capture the failure before reconnecting the cable; do not add an
automatic interface restart that would hide this distinction.

### Native peripheral series and hardware acceptance

The deployed fourteen-patch series was built from Linux source head
`25dafc9a120a` and LE integration commit `b15c469a10fe`. That complete image and
update are in `target/` with stem
`LibreELEC-SC2.aarch64-12.2-devel-20261019204550-b15c469` and `.sha256` files.
The source kernel Image, modules and board DTB compiled without warning/error
matches. Changed bindings passed `dt-doc-validate` after correcting the FD6551
single-cell indicator index schema. Source and FAT-shipped DTBs both passed
`dt-validate` without diagnostics. The shipped DTB additionally contains overlay
symbols and corresponding phandles; it is not byte-identical to the plain build.
The uImage remains uncompressed with load/entry `0x02000000`, below the existing
size bound. Image and update SHA-256 checks passed.

The final shipped SYSTEM and initramfs ran on a private image in QEMU using the
existing Linux 6.18.57 diagnostic kernel and virtual block/network devices.
The production first-boot resize requested a reboot; the second boot mounted
read-only `/flash` and SquashFS root, writable `/storage`, and started ConnMan
and SSH without failed units in the observation. `aplay` and `amixer` reported
1.2.11; `speaker-test` completed 48 kHz S16_LE stereo through ALSA's **null** PCM.
`ir-keytable` reported 1.30.1. `modetest -M meson -c` ran and correctly could not
open a Meson device in this VM. This checks installed userspace, not SC2 DRM,
audio DMA, IR reception, GPIO or any physical output. The private disk was removed;
logs remain in `Work/sc2-peripheral-smoke-16lejfj4/` at the workspace root.

| Item | Implemented path | Physical acceptance boundary |
| --- | --- | --- |
| IR | Native SC2 IR clock/GPIOD_5, NEC decoder and all 51 stock map_0 keys | Requires actual reception from the bundled handset; map_0 is a stock selection, not observed handset identity |
| Status LED | GPIOD_11 active high, standard GPIO LED | On/off visibility not observed with the new kernel |
| Front panel | Native FD6551 over GPIOA_14 DAT/GPIOA_15 CLK, standard linedisp/LED APIs | Corrected controller-relative pin translation; physical IC/segment/brightness acceptance pending |
| HDMI video | Native SC2 VPU, actual CLKCTRL/ANACTRL/PWRCTRL, DWC `fe300000`, TOP `fe308000`; corrected by patch 0015, CMA in 0016 | Production image boots with 256 MiB CMA, native 8,294,400-byte framebuffer and 1080p60 page flips. Actual HDMI pixels and TCL direct connection remain unverified |
| Digital audio | Powered domain 10, FRDDR A to optical SPDIF on H4; FRDDR B to TDM-C and real DW-HDMI codec | Requires DMA/IRQ growth, 48 kHz stereo receiver/channel checks and repeated start/stop; HDMI ELD/hotplug separately |
| 3.5mm AV audio | Native SC2 analog glue implemented, board analog path not activated | T9015 AVDD rail identity and actual positive/negative output-to-jack wiring are absent from stock data; no invented supply or headphone amplifier |
| USB2 | Existing stock low-power/free-clock quirks retained | Prior physical boot and bounded file readback passed; not a USB3 result |
| Blue USB3 port | Mode-aware combo-PHY reference sequencing and candidate super-speed cap; original USB2 quirks retained | Owner's earlier boot failure remains ground truth; no distinct PHY failure or post-change SuperSpeed transfer established |
| Native TF slot | SD_EMMC_B, GPIOC_6 active-low detect, 4-bit/50 MHz, no 1.8 V switching | Current mmc0 is bound but detect is high and no native MMC card is present; USB card-reader storage is not this slot |
| eMMC | Controller C, 8-bit/52 MHz, early type-MMC force_ro policy | Identity/capacity/partition inventory only after verified block RO; raw ioctls are not protected |
| WiFi/Bluetooth | rtw88 SDIO, native H5/btrtl, shared PWM_E LPO and unmodified upstream firmware | ROM identity, RF/coexistence, calibrated LPO and paired input pending |
| GPU/CEC | Native SC2 Panfrost match and clocked CECB/notifier | Binding is not rendering, CEC interoperability or TCL cold-boot persistence |
| CPU/thermal | Exact stock OPP/PWM_J rail, checked CPU/DSU sequencing, calibrated CPU/DDR zones | Inherited waveform/clock topology, trim accuracy, full transitions and cooling remain physical gates |

Local-series `checkpatch.pl` reported zero errors and nineteen warnings:
six binding/source patch-separation warnings, one new-file MAINTAINERS warning,
and twelve lines over 100 columns. The project's required Change-Id and absent
sign-off were excluded explicitly. This is not a warning-clean upstream submission;
no existing commit history was rewritten to conceal the findings.

The session's `/dev/ttyUSB0` entry was observed as a mode-000 empty regular file,
not an accessible character TTY. `stty` returned permission denied and
noninteractive sudo required a password. Do not chmod this placeholder and call
it a recovered console. The owner operates the 921600-baud console externally;
the assistant only notifies before reboot and does not verify TTL access.
The existing removable boot files were backed up without changing them to
`Work/sc2-pre-peripheral-boot-backup.tar` at the workspace root, SHA-256
`54a503c3fa1fba847767c3d30ec85e613c2c6612bc7f1166d0e101432c186afa`.

The verified update TAR was staged at
`/storage/.update/LibreELEC-SC2.aarch64-12.2-devel-20261019204550-b15c469.tar`
on the board: 69,683,200 bytes, SHA-256
`27b3b77bb21ddc4799c88e997b972a588af0617427eccde85ac94def4b5d7dee`.
It was uploaded under a non-update suffix, checked, fsynced and published only
after identity matched. `/flash` was still read-only at staging. After notifying
the owner, SSH `sync && systemctl reboot` returned success. The owner's log
confirms KERNEL/SYSTEM checksum checks, both payloads updated to 100%, boot-file
update and cleanup complete, and the standard automatic reboot.

The next USB boot loaded the 24,793,664-byte kernel and 16,614-byte DTB, reaching
Linux 6.18.57 built at 20:49:04. IR registered and the GPIO-I2C panel bus probed;
these are not key reception or panel visibility results. The owner confirms a
hang at the CVBS informational message. That probe returns zero without a CVBS
connector; HDMI component binding follows. After the owner read back the corrected
RAM DTB and ran `bootm 0x12000000 - 0x20000000`, the same kernel detected HDMI TX
v2.01a, registered DDC, bound the component and reached the LibreELEC login prompt.
SSH is available again; `systemctl is-system-running` reports `running`.

Keep the known-good USB2 boot medium connected. No deployment command writes
eMMC firmware or saves U-Boot environment, but the original firmware itself logs
`Saving Environment to STORAGE... OK`; do not claim zero writes across boot.
Discover actual device/control names before
operating them: `ir-keytable`, `modetest -M meson -c -p`, `aplay -l`, `amixer
controls`, DRM connector status/EDID, and the FD6551 parent's `linedisp.N/message`.
Use `1234`, brightness 1/4/8/off, and individual panel icons for visual checks;
inspect the real ALSA routes before sending stereo PCM. Never connect headphones
on the assumption that T9015 differential line outputs constitute a headphone amp.

CEC remains disabled and no forced EDID or HPD/+5V manipulation was added.
This series does not establish or repair TCL picture-setting persistence.

#### HDMI bank correction and RAM-only recovery

Kernel commit `988e7b095ee0`, exported as patch 0015, corrects DWC to
`0xfe300000` and TOP to `0xfe308000`. The vendor DT names those slots
`hdmitxtop` and `hdmitxdwc`, respectively, but the driver uses numeric indices:
`reg_ops.h` maps TOP to resource 3 and DWC to resource 4; `reg_ops.c` performs
word TOP accesses and byte DWC accesses through those indices. Slot names are
not programming authority. The previous reversed-map claim was wrong.

The corrected source DTB and binding passed compilation, `dt-doc-validate` and
`dt-validate`. A disposable copy of the shipped DTB accepted and read back the
same property replacement used below; it was removed afterwards. The standalone
corrected DTB is `target/meson-sc2-hk1-rbox-x4-hdmi-banks.dtb` (13,574 bytes),
SHA-256 `0368e79b3cf7228592ae78a496287258f0426670c8fe0052cb10a780ee0ef4d5`.
It lacks the image build's extra overlay symbols; that explains its smaller size.
The corrected mapping is now included in the complete CMA-enabled image and
update described below; the historical b15c469 image remains unchanged.

For an owner-operated recovery, notify before the power cycle, interrupt vendor
autoboot, and enter commands one at a time. With the observed single USB storage
device, use `usb 0:1`; stop on any load or FDT error:

```text
usb start
fatload usb 0:1 0x21000000 uEnv.ini
env import -t 0x21000000 ${filesize} bootargs
setenv bootargs "${bootargs} drm.debug=0x06"
fatload usb 0:1 0x12000000 KERNEL
fatload usb 0:1 0x20000000 amlogic/meson-sc2-hk1-rbox-x4.dtb
fdt addr 0x20000000
fdt resize 0x10000
fdt set /soc/hdmi-tx@fe308000 reg "<0x0 0xfe300000 0x0 0x8000 0x0 0xfe308000 0x0 0x8000>"
fdt print /soc/hdmi-tx@fe308000 reg
```

Check the printed `reg` cells before continuing. The old unit-address path is
intentional: this edits the deployed old DTB in RAM, preserving its phandles.
After successful readback, the complete boot command must include the FDT:

```text
setenv fdt_high ffffffffffffffff
bootm 0x12000000 - 0x20000000
```

Do not rerun the removable script after editing: it would reload the old DTB.
One owner attempt hit `Unknown command 'fdtfdt'` and ran `bootm 0x12000000 -`
without the third argument; firmware selected the FDT at `0x01000000`, not the
loaded `0x20000000` image. That attempt did not exercise the bank correction.

#### Native framebuffer allocation and CMA

The successful RAM-corrected boot reads an EDID named `HD TO USB`, with preferred
1920x1080 at 60 Hz and basic audio support. This is not a TCL-direct observation.
`modetest -M meson -c -p` opens the native DRM device and enumerates the connector,
CRTC and planes; the connector is connected but disabled, with no active framebuffer.
The kernel reports `failed to allocate buffer with size 8294400` followed by
`fbdev: Failed to setup emulation (ret=-12)`. Its embedded config has
`# CONFIG_CMA is not set`; `/proc/meminfo` has no CMA counters despite over 3 GiB
available RAM. Do not treat this as total RAM exhaustion or fix it by reducing
the display resolution, disabling fbdev, or merely appending `cma=` to a kernel
without CMA support.

Kernel commit `27d55aa73dee`, exported as patch 0016, adds the same reusable
256 MiB, 4 MiB-aligned `shared-dma-pool` used by `meson-g12-common.dtsi`.
Dynamic placement excludes existing firmware/secure-monitor/board reservations;
movable allocations can use idle CMA pages. The SC2 kernel config now explicitly
enables `CONFIG_CMA` and `CONFIG_DMA_CMA`. This is Linux DMA allocation policy,
not a vendor Android multimedia heap or permission to reuse secure carveouts.
The old b15c469 kernel cannot gain this allocator from a DTB-only update.
Controller/DDC success does not establish scanout, visible color, audio playback,
or TCL picture-setting persistence. The production CMA reservation, native
framebuffer allocation and finite 1080p60 flip checks below have now passed;
visible output remains a separate physical acceptance gate.

#### CMA-enabled build and removable update

Linux source `27d55aa73dee` and LE integration `87eb356972` produced the complete
`target/LibreELEC-SC2.aarch64-12.2-devel-20261019215739-87eb356` artifact set:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `.img.gz` | 55,111,945 | `30e741a792d99174a385b32fd80761013bfab687121bd743557ac012c71dc9e2` |
| `.tar` | 69,754,880 | `44326faecfecc7d7cd0bea18859ee6463e14d92085f1d9fedf905a9cb609dbde` |
| `.kernel` | 24,865,344 | `9d5caa751b57cecedff55cc29dc3005572cb35fb55c267df2e59524fe483e93e` |
| `.system` | 44,290,048 | `0f7db34a9e24d345beb816d231445eb0e1086622258a0bf63bfc2771de85b5cd` |

The decompressed image has FAT32 and Linux partitions, with no firmware payload.
Its KERNEL, SYSTEM, DTB and removable script match the TAR byte for byte. TAR MD5
and artifact SHA-256 checks passed. The kernel's embedded config enables CMA and
DMA_CMA; uImage header/data CRCs, load/entry and size checks passed. The shipped
16,768-byte DTB has corrected DWC/TOP banks and a 256 MiB/4 MiB-aligned CMA node;
it passed `dt-validate` without diagnostics. Its SHA-256 is
`7945280afe7471e1b7f882966a2d7d6e67f9f580a36f70b7e4df87b160941674`.

The TAR is staged under `/storage/.update/` on the board with the hash above.
Before staging, current removable boot files were backed up to workspace-root
`Work/sc2-pre-cma-boot-backup.tar` (69,108,736 bytes), SHA-256
`8c3329a454c9d230d351da40066cb64eb42a022302acdb9f23447337193e457c`.
The old on-disk DTB still had reversed HDMI banks. To let that kernel reach the
standard updater, only its two differing register bytes were corrected, exactly
as in the successful owner-operated RAM boot; existing phandles were preserved.
The temporary bridge DTB is 16,614 bytes, SHA-256
`07c6168ab6cb6ba740f414ec977fc06db641826be3ba913ca1393cf86a0b37c6`.
KERNEL, SYSTEM and `uEnv.ini` hashes remained unchanged, including partition UUIDs.
`/flash` was remounted read-only after the DTB replacement. This is not a CMA
DTB-only upgrade: the standard updater must install the matching new kernel,
modules, SYSTEM and final DTB together.

The owner was notified before SSH `sync && systemctl reboot`; the command
returned zero. Later SSH confirmed the new production image and matching installed
KERNEL/SYSTEM/DTB hashes. The owner operates UART;
the assistant does not inspect TTL availability.

After reboot, invoke the existing USB removable script from vendor U-Boot:

```text
usb start
fatload usb 0:1 0x22000000 s905_autoscript
autoscr 0x22000000
```

Stop if any command fails. Do not interrupt the standard update. If the
vendor boot policy does not automatically return to USB after its automatic
reboot, invoke those same three commands again. No manual bank edit is needed
with the bridge DTB or the final updated DTB. Do not save the U-Boot environment.

The running production kernel reports a 256 MiB pool at
`0xe0800000..0xf07fffff`, outside the existing secure and firmware reservations.
`CmaTotal` is 262,144 kB and `CmaFree` was 250,188 kB before the pattern test.
`systemctl is-system-running` reports `running`, with no failed units in the
observed output. Native fbcon now owns `mesondrmfb`: 1920x1080, 32 bpp, XR24,
pitch 7,680, size 8,294,400 bytes, DMA address `0xe0a00000`. CRTC 42 is enabled
and active at 1920x1080@60 on HDMI-A-1 connector 34. The previous framebuffer
`-ENOMEM` is absent from this boot's log.

A finite `(sleep 12; echo) | modetest -M meson -s 34@42:1920x1080-60 -F smpte -v`
run programmed the SMPTE pattern and reported 60.45 Hz initially, then eleven
60.00 Hz intervals before completing. This exercises allocation, modesetting
and flip events; it is not captured HDMI pixels. The sink EDID remains `HD TO USB`,
not TCL. Host `/dev/video0` and `/dev/video1` are mode-000 empty regular files,
not usable capture devices; they were not chmodded or presented as video proof.
Logs are workspace-root `Work/sc2-cma-after-live-state.log` and
`Work/sc2-hdmi-pageflip.log`. Visible output still requires the owner's observation;
HDMI/optical audio, IR keys, LED/panel visibility and TCL persistence are not
established by these checks.

After the finite pattern run, the original fbcon framebuffer was active again.
The board still reported `running`; its log contained no allocation failure,
DRM flip/vblank timeout or module-version disagreement. The standard update
directory was empty. This observation is recorded in
`Work/sc2-cma-after-pageflip.log`.

The final SYSTEM/initramfs also completed actual first-boot resize, reboot,
read-only flash/root mounts, writable storage, SSH and tool execution in QEMU.
The historical diagnostic kernel could not load the new production modules;
its symbol-version failures are retained rather than suppressed. A fresh
diagnostic kernel used the current production configuration plus PL011/VirtIO,
with explicit `LOCALVERSION=` to match `6.18.57`. It loaded the shipped
`pkcs8_key_parser`, `nfnetlink` and `fuse` modules and reported ConnMan, SSH and
modules-load active with no failed units. Mirroring the production CMA node's
size/alignment onto the QEMU DTB initialized 256 MiB without a `cma=` override.
Logs, including both failed diagnostic attempts, remain in
`Work/sc2-cma-release-6mwgi0ap/`. The diagnostic kernel is not shipped; this VM
result does not replace the separate SC2 hardware observations above.

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

These QEMU checks do not execute the vendor U-Boot script or any SC2 peripheral.
The owner's later hardware log confirms Linux 6.18.57 reaching authenticated
921600-baud UART login after mounting USB-connected storage, superseding the
earlier console-handover and `mount_flash` failures. A later read-only SSH
observation confirms USB2-backed `/flash` and `/storage` mounts and Ethernet
negotiated at 1000 Mb/s full duplex on the running system. It does not establish
cold-boot networking or throughput. Ethernet PHY detection and owner-reported
recovery after cable reconnection are recorded above. Native
SD-slot operation, USB3, cold-boot Ethernet reliability, and memory-reservation
compatibility remain unverified. Those observations used the earlier headless
image; the later native-peripheral build and its still-pending hardware acceptance
are recorded separately above.
