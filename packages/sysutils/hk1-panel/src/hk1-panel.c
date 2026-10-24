// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HK1 RBOX X4 policy for native line-display and LED class interfaces.
 * Only /i2c-panel/display-controller@24 on hk1,rbox-x4 is owned here;
 * LED OF parents and native function names identify outputs; I2C bus numbers
 * are deliberately not part of the ABI.
 * HHMM is four ASCII glyphs, with the separate colon LED blinking at 1 Hz.
 * Default 24-hour time follows system timezone changes; --format 12 displays
 * 00 at midnight and 12 at noon because the panel has no AM/PM indicator.
 * Network icons mean physical Ethernet/WLAN operstate=up, not Internet access.
 * USB means nonempty block media below a USB mass-storage interface, whether
 * mounted or not (including the boot medium). Empty readers/hubs do not count.
 * Discovery and network/media refresh run every two seconds; time/colon every
 * half-second. No external processes, Android ABI or GPIO/I2C access is used.
 * Only changed outputs are written; missing/late devices remain discoverable.
 * SIGTERM/SIGINT blank owned text/icons and disable display illumination.
 * --root DIR prefixes sysfs for fixtures; --once leaves fixture outputs for
 * inspection, --time EPOCH pins wall time (colon on). These two options require
 * TZ may be set for deterministic fixtures; normal operation uses /etc/localtime.
 * A fixture is a userspace ABI test, not evidence of panel acknowledgement or
 * visible output.
 */
#define _XOPEN_SOURCE 700
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define PANEL "/sys/firmware/devicetree/base/i2c-panel/display-controller@24"
#define BOARD "/sys/firmware/devicetree/base/compatible"

enum { MESSAGE, DISPLAY, COLON, LAN, WLAN, USB, OUTPUTS };
struct output {
	char path[PATH_MAX];
	char value[16];
	int last_error;
};
static struct output outputs[OUTPUTS];
static char root[PATH_MAX];
static volatile sig_atomic_t stopping;

static void stop(int signo)
{
	(void)signo;
	stopping = 1;
}

static bool pathcat(char *out, const char *base, const char *suffix)
{
	int n = snprintf(out, PATH_MAX, "%s%s", base, suffix);
	return n >= 0 && n < PATH_MAX;
}

static ssize_t read_file(const char *path, char *buf, size_t size)
{
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	ssize_t n;
	if (fd < 0)
		return -1;
	n = read(fd, buf, size - 1);
	close(fd);
	if (n >= 0)
		buf[n] = '\0';
	return n;
}

static bool has_string(const char *path, const char *value)
{
	char buf[512];
	ssize_t n = read_file(path, buf, sizeof(buf));
	for (ssize_t i = 0; i < n;) {
		size_t len = strnlen(buf + i, (size_t)(n - i));
		if (len == strlen(value) && !memcmp(buf + i, value, len))
			return true;
		i += (ssize_t)len + 1;
	}
	return false;
}

static bool exists(const char *base, const char *suffix)
{
	char path[PATH_MAX];
	return pathcat(path, base, suffix) && access(path, F_OK) == 0;
}

static long long number(const char *base, const char *suffix, int radix)
{
	char path[PATH_MAX], buf[80], *end;
	if (!pathcat(path, base, suffix) || read_file(path, buf, sizeof(buf)) <= 0)
		return -1;
	errno = 0;
	long long value = strtoll(buf, &end, radix);
	if (errno || end == buf || (*end && *end != '\n'))
		return -1;
	return value;
}

static bool exact_node(const char *device, const char *expected)
{
	char path[PATH_MAX], actual[PATH_MAX], wanted[PATH_MAX];
	return pathcat(path, device, "/of_node") && realpath(path, actual) &&
		pathcat(wanted, root, expected) && !strcmp(actual, wanted);
}

/* led_classdev_register_ext() sets its fwnode after device_add(), so these
 * class devices need not expose of_node. Their device link still identifies
 * the exact controller. Names come from led_compose_name(): ":function".
 */
static bool panel_led(const char *device, const char *node, const char *function)
{
	char parent[PATH_MAX], path[PATH_MAX], expected[80];
	if (exact_node(device, node))
		return true;
	if (!pathcat(parent, device, "/device") || !exact_node(parent, PANEL) ||
	    !pathcat(path, root, node) || !pathcat(parent, path, "/function") ||
	    !has_string(parent, function))
		return false;
	const char *name = strrchr(device, '/');
	if (!name)
		return false;
	snprintf(expected, sizeof(expected), ":%s", function);
	name++;
	size_t len = strlen(expected);
	if (strncmp(name, expected, len))
		return false;
	name += len;
	if (!*name)
		return true;
	if (*name++ != '_' || !*name)
		return false;
	for (; *name; name++)
		if (*name < '0' || *name > '9')
			return false;
	return true;
}

static void select_output(unsigned int index, const char *device, const char *attr)
{
	char path[PATH_MAX], resolved[PATH_MAX];
	if (!pathcat(path, device, attr) || !realpath(path, resolved))
		return;
	/* A fixture may contain symlinks, but must never write outside its sysfs. */
	if (root[0] && (strncmp(resolved, root, strlen(root)) ||
			strncmp(resolved + strlen(root), "/sys/", 5)))
		return;
	if (!strcmp(resolved, outputs[index].path))
		return;
	strcpy(outputs[index].path, resolved);
	outputs[index].value[0] = '\0';
	outputs[index].last_error = 0;
}

static void discover(void)
{
	char base[PATH_MAX], device[PATH_MAX], path[PATH_MAX];
	bool found[OUTPUTS] = { false };
	DIR *dir;
	struct dirent *entry;

	if (!pathcat(base, root, "/sys/bus/i2c/devices"))
		return;
	dir = opendir(base);
	if (dir) {
		while ((entry = readdir(dir))) {
			if (entry->d_name[0] == '.' ||
			    snprintf(device, sizeof(device), "%s/%s", base, entry->d_name) >= PATH_MAX ||
			    !exact_node(device, PANEL) ||
			    !pathcat(path, device, "/of_node/compatible") ||
			    !has_string(path, "fdhisi,fd6551"))
				continue;
			DIR *children = opendir(device);
			struct dirent *child;
			if (!children)
				continue;
			while ((child = readdir(children))) {
				if (strncmp(child->d_name, "linedisp.", 9))
					continue;
				if (snprintf(path, sizeof(path), "%s/%s", device, child->d_name) >= PATH_MAX)
					continue;
				select_output(MESSAGE, path, "/message");
				found[MESSAGE] = true;
			}
			closedir(children);
		}
		closedir(dir);
	}

	static const char *nodes[] = { "/display", "/led@4", "/led@5", "/led@6", "/led@1" };
	static const char *functions[] = { "display", "colon", "lan", "wlan", "usb" };
	if (!pathcat(base, root, "/sys/class/leds"))
		return;
	dir = found[MESSAGE] ? opendir(base) : NULL;
	if (dir) {
		while ((entry = readdir(dir))) {
			if (entry->d_name[0] == '.' ||
			    snprintf(device, sizeof(device), "%s/%s", base, entry->d_name) >= PATH_MAX)
				continue;
			for (unsigned int i = DISPLAY; i < OUTPUTS; i++) {
				if (!pathcat(path, PANEL, nodes[i - DISPLAY]) ||
				    !panel_led(device, path, functions[i - DISPLAY]))
					continue;
				select_output(i, device, "/brightness");
				found[i] = true;
			}
		}
		closedir(dir);
	}
	for (unsigned int i = 0; i < OUTPUTS; i++) {
		if (!found[i]) {
			outputs[i].path[0] = '\0';
			outputs[i].value[0] = '\0';
			outputs[i].last_error = 0;
		}
	}
}

static void set_output(unsigned int index, const char *value)
{
	struct output *out = &outputs[index];
	if (!out->path[0] || !strcmp(out->value, value))
		return;
	int fd = open(out->path, O_WRONLY | O_CLOEXEC | O_TRUNC);
	if (fd < 0)
		return;
	size_t len = strlen(value);
	ssize_t written = write(fd, value, len);
	int saved = errno;
	close(fd);
	if (written == (ssize_t)len) {
		strcpy(out->value, value);
		out->last_error = 0;
	} else {
		int error = written < 0 ? saved : EIO;
		if (out->last_error != error)
			fprintf(stderr, "hk1-panel: write %s: %s\n", out->path,
				written < 0 ? strerror(error) : "short write");
		out->last_error = error;
	}
}

static void network(bool *lan, bool *wlan)
{
	char base[PATH_MAX], device[PATH_MAX], path[PATH_MAX], state[32];
	struct dirent *entry;
	*lan = *wlan = false;
	if (!pathcat(base, root, "/sys/class/net"))
		return;
	DIR *dir = opendir(base);
	if (!dir)
		return;
	while ((entry = readdir(dir))) {
		if (entry->d_name[0] == '.' ||
		    snprintf(device, sizeof(device), "%s/%s", base, entry->d_name) >= PATH_MAX ||
		    number(device, "/type", 10) != 1 || !exists(device, "/device") ||
		    !pathcat(path, device, "/operstate") ||
		    read_file(path, state, sizeof(state)) <= 0 || strcmp(state, "up\n"))
			continue;
		if (exists(device, "/phy80211") || exists(device, "/wireless"))
			*wlan = true;
		else
			*lan = true;
	}
	closedir(dir);
}

/* Require actual nonempty block media below a USB mass-storage interface.
 * Root hubs, empty card readers, HID and USB network interfaces do not qualify. */
static bool usb_media(void)
{
	char base[PATH_MAX], block[PATH_MAX], path[PATH_MAX], resolved[PATH_MAX];
	struct dirent *entry;
	if (!pathcat(base, root, "/sys/class/block"))
		return false;
	DIR *dir = opendir(base);
	if (!dir)
		return false;
	bool present = false;
	while (!present && (entry = readdir(dir))) {
		if (entry->d_name[0] == '.' ||
		    snprintf(block, sizeof(block), "%s/%s", base, entry->d_name) >= PATH_MAX ||
		    number(block, "/size", 10) <= 0 || !realpath(block, resolved))
			continue;
		while (strlen(resolved) > strlen(root) + 4) {
			char subsystem[PATH_MAX];
			if (pathcat(path, resolved, "/subsystem") && realpath(path, subsystem) &&
			    pathcat(path, root, "/sys/bus/usb") && !strcmp(subsystem, path) &&
			    number(resolved, "/bInterfaceClass", 16) == 8) {
				present = true;
				break;
			}
			char *slash = strrchr(resolved, '/');
			if (!slash)
				break;
			*slash = '\0';
		}
	}
	closedir(dir);
	return present;
}

int main(int argc, char **argv)
{
	bool once = false, fixed = false;
	int format = 24;
	time_t fixed_time = 0;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--once"))
			once = true;
		else if (!strcmp(argv[i], "--root") && i + 1 < argc) {
			if (!realpath(argv[++i], root) || !strcmp(root, "/")) {
				fprintf(stderr, "hk1-panel: fixture root must be an existing non-root directory\n");
				return 2;
			}
		} else if (!strcmp(argv[i], "--format") && i + 1 < argc) {
			const char *arg = argv[++i];
			if (!strcmp(arg, "12"))
				format = 12;
			else if (!strcmp(arg, "24"))
				format = 24;
			else
				goto usage;
		} else if (!strcmp(argv[i], "--time") && i + 1 < argc) {
			char *end;
			errno = 0;
			long long value = strtoll(argv[++i], &end, 10);
			if (errno || !argv[i][0] || *end)
				goto usage;
			fixed_time = (time_t)value;
			if ((long long)fixed_time != value)
				goto usage;
			fixed = true;
		} else {
usage:
			fprintf(stderr, "usage: hk1-panel [--format 12|24] [--root FIXTURE [--once] [--time EPOCH]]\n");
			return 2;
		}
	}
	if ((once || fixed) && !root[0])
		goto usage;
	char path[PATH_MAX];
	if (!pathcat(path, root, BOARD) || !has_string(path, "hk1,rbox-x4"))
		return 0;
	signal(SIGTERM, stop);
	signal(SIGINT, stop);
	unsigned int tick = 0;
	bool lan = false, wlan = false, usb = false;
	do {
		struct timespec now;
		if (clock_gettime(CLOCK_REALTIME, &now))
			return 1;
		if (fixed) {
			now.tv_sec = fixed_time;
			now.tv_nsec = 0;
		}
		if (!(tick++ % 4)) {
			discover();
			network(&lan, &wlan);
			usb = usb_media();
		}
		tzset();
		struct tm local;
		if (!localtime_r(&now.tv_sec, &local))
			return 1;
		int hour = local.tm_hour;
		if (format == 12 && hour > 12)
			hour -= 12;
		char message[16];
		snprintf(message, sizeof(message), "%02d%02d", hour, local.tm_min);
		set_output(MESSAGE, message);
		set_output(DISPLAY, "8\n");
		set_output(COLON, now.tv_nsec < 500000000 ? "1\n" : "0\n");
		set_output(LAN, lan ? "1\n" : "0\n");
		set_output(WLAN, wlan ? "1\n" : "0\n");
		set_output(USB, usb ? "1\n" : "0\n");
		if (once)
			return 0;
		/* Relative sleep survives wall-clock steps; recompute local time every tick. */
		struct timespec delay = { .tv_nsec = 500000000 - now.tv_nsec % 500000000 };
		while (!stopping && nanosleep(&delay, &delay) && errno == EINTR)
			;
	} while (!stopping);
	set_output(MESSAGE, "    ");
	for (unsigned int i = DISPLAY; i < OUTPUTS; i++)
		set_output(i, "0\n");
	return 0;
}
