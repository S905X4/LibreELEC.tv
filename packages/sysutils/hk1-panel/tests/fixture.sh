#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Host-side ABI smoke: sh fixture.sh /path/to/hk1-panel /path/to/fresh-fixture
# All generated paths stay below the caller-provided fresh fixture directory.
set -eu
bin=$(realpath "$1")
mkdir "$2"
r=$(realpath "$2")
node="$r/sys/firmware/devicetree/base/i2c-panel/display-controller@24"
mkdir -p "$node" "$r/sys/bus/i2c/devices" "$r/sys/class/leds" \
  "$r/sys/class/net" "$r/sys/class/block" "$r/sys/bus/usb"
printf 'hk1,rbox-x4\000amlogic,sc2\000' > "$r/sys/firmware/devicetree/base/compatible"
printf 'fdhisi,fd6551\000' > "$node/compatible"
# Start before any class devices exist: the daemon must discover them later.
TZ=UTC0 "$bin" --root "$r" --time 47040 &
pid=$!
trap 'kill "$pid" 2>/dev/null || :; wait "$pid" 2>/dev/null || :' EXIT HUP INT TERM
panel="$r/sys/bus/i2c/devices/9-0024"
mkdir -p "$panel/linedisp.7"
ln -s "$node" "$panel/of_node"
: > "$panel/linedisp.7/message"
for pair in display:display colon:led@4 lan:led@5 wlan:led@6 usb:led@1; do
  name=${pair%:*}; child=${pair#*:}
  mkdir -p "$node/$child" "$r/sys/class/leds/:$name"
  printf '%s\000' "$name" > "$node/$child/function"
  ln -s "$panel" "$r/sys/class/leds/:$name/device"
  : > "$r/sys/class/leds/:$name/brightness"
done
# Names deliberately have no eth/wlan assumptions.
for name in cable radio; do
  d="$r/sys/class/net/$name"
  mkdir -p "$d/device"
  printf '1\n' > "$d/type"
  printf 'up\n' > "$d/operstate"
done
mkdir "$r/sys/class/net/radio/phy80211"
# A nonempty USB storage block medium, with a real subsystem link.
media="$r/sys/devices/usb-fixture/1-1:1.0/block/disk"
mkdir -p "$media"
ln -s "$r/sys/bus/usb" "$r/sys/devices/usb-fixture/1-1:1.0/subsystem"
printf '08\n' > "$r/sys/devices/usb-fixture/1-1:1.0/bInterfaceClass"
printf '2048\n' > "$media/size"
ln -s "$media" "$r/sys/class/block/unusual-disk"
attempts=0
until test "$(cat "$panel/linedisp.7/message")" = 1304 &&
      test "$(cat "$r/sys/class/leds/:lan/brightness")" = 1 &&
      test "$(cat "$r/sys/class/leds/:wlan/brightness")" = 1 &&
      test "$(cat "$r/sys/class/leds/:usb/brightness")" = 1; do
  kill -0 "$pid"
  attempts=$((attempts + 1))
  if test "$attempts" -ge 20; then
    printf 'panel did not converge within 20 seconds\n' >&2
    exit 1
  fi
  sleep 1
done
kill -TERM "$pid"
wait "$pid"
trap - EXIT HUP INT TERM
test "$(cat "$panel/linedisp.7/message")" = '    '
for name in display colon lan wlan usb; do
  test "$(cat "$r/sys/class/leds/:$name/brightness")" = 0
done
# Deterministic minute, timezone and stock-format boundary cases.
TZ=UTC0 "$bin" --root "$r" --once --time 47100 --format 12
test "$(cat "$panel/linedisp.7/message")" = 0105
TZ=UTC-2 "$bin" --root "$r" --once --time 47100
test "$(cat "$panel/linedisp.7/message")" = 1505
TZ=UTC0 "$bin" --root "$r" --once --time 0 --format 12
test "$(cat "$panel/linedisp.7/message")" = 0000
TZ=UTC0 "$bin" --root "$r" --once --time 43200 --format 12
test "$(cat "$panel/linedisp.7/message")" = 1200
# Down links and an empty USB reader must clear their icons.
printf 'down\n' > "$r/sys/class/net/cable/operstate"
printf 'dormant\n' > "$r/sys/class/net/radio/operstate"
printf '0\n' > "$media/size"
TZ=UTC0 "$bin" --root "$r" --once --time 43200
for name in lan wlan usb; do
  test "$(cat "$r/sys/class/leds/:$name/brightness")" = 0
done
# A hub-like ancestor is insufficient even with a nonzero block size.
printf '2048\n' > "$media/size"
printf '09\n' > "$r/sys/devices/usb-fixture/1-1:1.0/bInterfaceClass"
TZ=UTC0 "$bin" --root "$r" --once --time 43200
test "$(cat "$r/sys/class/leds/:usb/brightness")" = 0
printf 'fixture smoke passed\n'
