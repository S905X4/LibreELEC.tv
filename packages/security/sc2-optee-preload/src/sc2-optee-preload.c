// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SC2 video firmware preload using Linux TEE UAPI and OP-TEE client 4.8 RPC.
 * Implements LOAD_TA and transient SHM_ALLOC/SHM_FREE requests only. This is
 * not a general tee-supplicant or a security boundary for secure-world code.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/tee.h>
#include <openssl/evp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <tee_client_api.h>
#include <time.h>
#include <unistd.h>

#define TA_NAME "526fc4fc-7ee6-4a12-96e3-83da9565bce8.ta"
#define TA_BYTES 67776u
#define PACKAGE_BYTES 1871104u
#define CHUNK_BYTES 0x20000u
#define RPC_LOAD_TA 0u
#define RPC_RPMB 1u
#define RPC_FS 2u
#define RPC_SHM_ALLOC 6u
#define RPC_SHM_FREE 7u
#define RPC_SQL_FS 8u
/* Native Linux optee_rpc_cmd.h; forwarded when RPMB routing is disabled. */
#define RPC_RPMB_PROBE_RESET 22u
#define RPC_RPMB_PROBE_NEXT 23u
#define RPC_RPMB_FRAMES 24u
#define RPC_PARAMS 16u
/* Bounded transient RPC memory, not a limit on the package streaming size. */
#define SHM_SLOTS 32u
#define SHM_BUDGET (16u * 1024u * 1024u)

static const unsigned char ta_uuid[16] = {
	0x52, 0x6f, 0xc4, 0xfc, 0x7e, 0xe6, 0x4a, 0x12,
	0x96, 0xe3, 0x83, 0xda, 0x95, 0x65, 0xbc, 0xe8
};
static const unsigned char ta_sha256[32] = {
	0x5b, 0xf5, 0x91, 0x44, 0x07, 0x6a, 0xf7, 0x8c,
	0x61, 0x2b, 0xd0, 0x4b, 0x51, 0xa9, 0x22, 0x15,
	0xe3, 0xc1, 0x5a, 0xb1, 0xb8, 0xd8, 0xee, 0xb0,
	0x13, 0x1c, 0x2e, 0x03, 0x36, 0x69, 0x75, 0x68
};
static const unsigned char package_sha256[32] = {
	0x03, 0x2b, 0x96, 0xc4, 0x98, 0xce, 0xe3, 0x71,
	0xc3, 0xeb, 0x37, 0xd6, 0x0b, 0x1e, 0xae, 0x0f,
	0x25, 0xe3, 0x2c, 0x0b, 0x96, 0x95, 0xa1, 0xd9,
	0x17, 0x35, 0x40, 0x86, 0x62, 0x30, 0x6c, 0xe4
};

struct rpc_buffer {
	/* recv.func/send.ret, followed by num_params, have identical layout. */
	uint32_t code;
	uint32_t count;
	struct tee_ioctl_param params[RPC_PARAMS];
};

struct rpc_shm {
	void *buffer;
	size_t size;
	int fd;
	int id;
};

struct provider {
	int fd;
	size_t used;
	struct rpc_shm shms[SHM_SLOTS];
	const unsigned char *ta;
};

static volatile sig_atomic_t stopping;

static void stop_provider(int sig)
{
	(void)sig;
	stopping = 1;
}

/* Snapshot bytes before hashing: neither later replacement nor modification of
 * an input file can change the bytes subsequently sent to secure world.
 */
static unsigned char *read_asset(int fd, size_t length,
				 const unsigned char expected[32])
{
	struct stat st;
	unsigned char digest[EVP_MAX_MD_SIZE], *bytes;
	unsigned int digest_length = 0;
	size_t done = 0;

	if (fstat(fd, &st) || !S_ISREG(st.st_mode) ||
	    st.st_size != (off_t)length) {
		fprintf(stderr, "asset must be a regular file of exactly %zu bytes\n", length);
		return NULL;
	}
	bytes = malloc(length);
	if (!bytes)
		return NULL;
	while (done < length) {
		ssize_t n = read(fd, bytes + done, length - done);

		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			goto fail;
		done += (size_t)n;
	}
	if (!EVP_Digest(bytes, length, digest, &digest_length, EVP_sha256(), NULL) ||
	    digest_length != 32 || memcmp(digest, expected, 32)) {
		fprintf(stderr, "original asset SHA-256 mismatch\n");
		goto fail;
	}
	return bytes;
fail:
	free(bytes);
	return NULL;
}

/* Only the native SC2 boot_complete attribute accepts this fixed write. The
 * kernel consumes the notification once even if this process loses the reply.
 * Never retry a short/interrupted/failed write: retain the raw status instead.
 */
static int notify_boot_complete(const char *path)
{
	char state[192];
	unsigned long a0, a1, a2, a3;
	ssize_t written, length;
	int fd, write_error, end = 0;

	fd = open(path, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0) {
		perror("open SC2 boot_complete attribute");
		return 1;
	}
	written = write(fd, "1\n", 2);
	write_error = errno;
	length = pread(fd, state, sizeof(state) - 1, 0);
	close(fd);
	if (length > 0) {
		state[length] = '\0';
		fprintf(stderr, "boot-complete raw status: %s", state);
	} else {
		fprintf(stderr, "boot-complete raw status unavailable; do not retry notification\n");
		return 1;
	}
	if (written != 2) {
		fprintf(stderr, "boot-complete write %s; no notification retry: %s\n",
			written < 0 ? "failed" : "was short",
			written < 0 ? strerror(write_error) : "uncertain delivery");
		return 1;
	}
	if (sscanf(state, "attempted a0=%lx a1=%lx a2=%lx a3=%lx%n",
		   &a0, &a1, &a2, &a3, &end) != 4 || state[end] != '\n' ||
	    state[end + 1] || a0) {
		fprintf(stderr, "boot-complete did not report secure success; no notification retry\n");
		return 1;
	}
	return 0;
}

static void release_shm(struct provider *p, struct rpc_shm *shm)
{
	/* Keep the mapping alive until its kernel reference closes. */
	close(shm->fd);
	if (munmap(shm->buffer, shm->size))
		perror("munmap RPC SHM");
	p->used -= shm->size;
	memset(shm, 0, sizeof(*shm));
}

static uint32_t allocate_shm(struct provider *p, size_t count,
			      struct tee_ioctl_param *v)
{
	struct tee_ioctl_shm_alloc_data data = { 0 };
	struct rpc_shm *shm = NULL;
	size_t size, i;

	/* Linux rpc.c sends application SHM type 0, VALUE_INOUT(size,id). */
	if (count != 1 || v->attr != TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT || v->a)
		return TEEC_ERROR_BAD_PARAMETERS;
	if (!v->b || v->b > SHM_BUDGET - p->used)
		return TEEC_ERROR_OUT_OF_MEMORY;
	size = (size_t)v->b;
	for (i = 0; i < SHM_SLOTS; i++) {
		if (!p->shms[i].buffer) {
			shm = p->shms + i;
			break;
		}
	}
	if (!shm)
		return TEEC_ERROR_OUT_OF_MEMORY;
	/* Match the SC2 driver's negotiated reserved pool. External registered
	 * page lists can fail in the factory TA loader with OUT_OF_MEMORY even
	 * when normal-world allocation succeeds. Allocation failure stays fatal;
	 * do not retry with a different transport or fabricated RPC response.
	 */
	data.size = size;

	shm->fd = ioctl(p->fd, TEE_IOC_SHM_ALLOC, &data);
	if (shm->fd < 0) {
		perror("allocate RPC SHM");
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	shm->buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
			   shm->fd, 0);
	if (shm->buffer == MAP_FAILED) {
		perror("map RPC SHM");
		close(shm->fd);
		shm->buffer = NULL;
		return TEEC_ERROR_OUT_OF_MEMORY;
	}
	memset(shm->buffer, 0, size);
	shm->id = data.id;
	shm->size = size;
	p->used += size;
	v->c = (uint32_t)shm->id;
	return TEEC_SUCCESS;
}

static uint32_t free_shm(struct provider *p, size_t count,
			  struct tee_ioctl_param *v)
{
	size_t i;

	if (count != 1 || v->attr != TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT ||
	    v->a || v->b > INT_MAX)
		return TEEC_ERROR_BAD_PARAMETERS;
	for (i = 0; i < SHM_SLOTS; i++) {
		if (p->shms[i].buffer && p->shms[i].id == (int)v->b) {
			release_shm(p, p->shms + i);
			return TEEC_SUCCESS;
		}
	}
	return TEEC_ERROR_BAD_PARAMETERS;
}

static uint32_t load_ta(struct provider *p, size_t count,
			struct tee_ioctl_param *v)
{
	struct rpc_shm *shm = NULL;
	uint64_t capacity;
	size_t i;

	if (count != 2 || v[0].attr != TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT ||
	    v[1].attr != TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT)
		return TEEC_ERROR_BAD_PARAMETERS;
	/* OP-TEE packs the UUID octets into the first 16 bytes of value a,b. */
	if (memcmp(&v[0].a, ta_uuid, sizeof(ta_uuid)))
		return TEEC_ERROR_ITEM_NOT_FOUND;
	capacity = v[1].b;
	if (v[1].c == TEE_MEMREF_NULL) {
		if (capacity || v[1].a)
			return TEEC_ERROR_BAD_PARAMETERS;
		v[1].b = TA_BYTES;
		return TEEC_SUCCESS;
	}
	if (v[1].c > INT_MAX)
		return TEEC_ERROR_BAD_PARAMETERS;
	for (i = 0; i < SHM_SLOTS; i++) {
		if (p->shms[i].buffer && p->shms[i].id == (int)v[1].c) {
			shm = p->shms + i;
			break;
		}
	}
	if (!shm || v[1].a > shm->size || capacity > shm->size - v[1].a)
		return TEEC_ERROR_BAD_PARAMETERS;
	v[1].b = TA_BYTES;
	if (capacity < TA_BYTES)
		return TEEC_ERROR_SHORT_BUFFER;
	memcpy((unsigned char *)shm->buffer + v[1].a, p->ta, TA_BYTES);
	return TEEC_SUCCESS;
}

static uint32_t dispatch(struct provider *p, struct rpc_buffer *request)
{
	switch (request->code) {
	case RPC_LOAD_TA:
		return load_ta(p, request->count, request->params);
	case RPC_SHM_ALLOC:
		return allocate_shm(p, request->count, request->params);
	case RPC_SHM_FREE:
		return free_shm(p, request->count, request->params);
	case RPC_RPMB:
	case RPC_FS:
	case RPC_SQL_FS:
	case RPC_RPMB_PROBE_RESET:
	case RPC_RPMB_PROBE_NEXT:
	case RPC_RPMB_FRAMES:
		/* Deny every operation, including reads, enumeration and provisioning. */
		return TEEC_ERROR_ACCESS_DENIED;
	default:
		/* Includes plugins, sockets, profiling, vendor/raw MMC RPC extensions. */
		return TEEC_ERROR_NOT_SUPPORTED;
	}
}

static int serve(const char *device, const unsigned char *ta, int ready_fd)
{
	struct provider p = { .fd = -1, .ta = ta };
	struct tee_ioctl_version_data version = { 0 };
	struct sigaction action = { .sa_handler = stop_provider };
	sigset_t empty;
	int status = 1;
	size_t i;

	sigemptyset(&action.sa_mask);
	if (sigaction(SIGTERM, &action, NULL) ||
	    sigaction(SIGINT, &action, NULL) || sigaction(SIGHUP, &action, NULL))
		goto out;
	sigemptyset(&empty);
	if (sigprocmask(SIG_SETMASK, &empty, NULL))
		goto out;
	p.fd = open(device, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
	if (p.fd < 0 || ioctl(p.fd, TEE_IOC_VERSION, &version)) {
		perror("open/version privileged OP-TEE device");
		goto out;
	}
	if (version.impl_id != TEE_IMPL_ID_OPTEE ||
	    !(version.impl_caps & TEE_OPTEE_CAP_TZ) ||
	    !(version.gen_caps & TEE_GEN_CAP_PRIVILEGED)) {
		fprintf(stderr, "not a privileged native OP-TEE TZ device\n");
		goto out;
	}
	if (write(ready_fd, "R", 1) != 1)
		goto out;
	close(ready_fd);
	ready_fd = -1;
	while (!stopping) {
		struct rpc_buffer request = { .count = RPC_PARAMS };
		struct tee_ioctl_buf_data data = {
			.buf_ptr = (uintptr_t)&request, .buf_len = sizeof(request)
		};
		uint32_t command, result;

		/* Synchronous, non-meta protocol: no detached threads or nested RPC. */
		if (ioctl(p.fd, TEE_IOC_SUPPL_RECV, &data)) {
			if (errno == EINTR)
				continue;
			perror("TEE_IOC_SUPPL_RECV");
			goto out;
		}
		if (request.count > RPC_PARAMS) {
			fprintf(stderr, "RPC parameter count exceeds receive buffer\n");
			goto out;
		}
		command = request.code;
		result = dispatch(&p, &request);
		fprintf(stderr, "rpc command=%" PRIu32 " params=%" PRIu32
			" result=%08" PRIx32 "\n", command, request.count, result);
		request.code = result;
		data.buf_len = sizeof(uint32_t) * 2 +
			request.count * sizeof(struct tee_ioctl_param);
		if (ioctl(p.fd, TEE_IOC_SUPPL_SEND, &data)) {
			perror("TEE_IOC_SUPPL_SEND");
			goto out;
		}
	}
	status = 0;
out:
	if (ready_fd >= 0)
		close(ready_fd);
	/* Drop our device and SHM references; optee_release_supp completes queued
	 * requests with communication errors when the last context ref releases.
	 * Keep each allocation mapped until its individual fd is closed.
	 */
	if (p.fd >= 0)
		close(p.fd);
	for (i = 0; i < SHM_SLOTS; i++)
		if (p.shms[i].buffer)
			release_shm(&p, p.shms + i);
	return status;
}

static int preload(const unsigned char *package, const char *priv_device)
{
	const TEEC_UUID uuid = {
		0x526fc4fc, 0x7ee6, 0x4a12,
		{ 0x96, 0xe3, 0x83, 0xda, 0x95, 0x65, 0xbc, 0xe8 }
	};
	TEEC_Context context;
	TEEC_Session session;
	TEEC_SharedMemory memory = { .size = CHUNK_BYTES, .flags = TEEC_MEM_INPUT };
	TEEC_Operation operation = { 0 };
	uint32_t origin = 0;
	TEEC_Result result;
	size_t offset = 0;
	int status = 1;
	sigset_t empty;
	struct stat actual, expected_device;
	char client_device[] = "/dev/tee0";

	sigemptyset(&empty);
	if (sigprocmask(SIG_SETMASK, &empty, NULL))
		return 1;
	result = TEEC_InitializeContext("optee-tz", &context);
	printf("initialize result=%08" PRIx32 "\n", result);
	if (result != TEEC_SUCCESS)
		return 1;
	/* Keep libteec's normal ABI initialization, but do not silently connect
	 * to a different OP-TEE instance than the explicitly selected provider.
	 * imp.fd is the pinned libteec 4.8 context's native device descriptor.
	 */
	client_device[8] = priv_device[12];
	if (stat(client_device, &expected_device) || fstat(context.imp.fd, &actual) ||
	    !S_ISCHR(actual.st_mode) || actual.st_rdev != expected_device.st_rdev) {
		fprintf(stderr, "libteec selected a different TEE than %s\n", client_device);
		goto out_context;
	}
	/* The pinned client otherwise registers malloc-backed pages. Use the
	 * same negotiated reserved allocation as RPC for package chunks too;
	 * do not retry registration failures or change the kernel capabilities.
	 */
	context.imp.reg_mem = false;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
					      TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = PACKAGE_BYTES;
	operation.params[1].value.a = 0x100;
	result = TEEC_OpenSession(&context, &session, &uuid, TEEC_LOGIN_PUBLIC,
				 NULL, &operation, &origin);
	printf("open result=%08" PRIx32 " origin=%" PRIu32 "\n", result, origin);
	if (result != TEEC_SUCCESS)
		goto out_context;
	result = TEEC_AllocateSharedMemory(&context, &memory);
	printf("allocate result=%08" PRIx32 "\n", result);
	if (result != TEEC_SUCCESS)
		goto out_session;
	while (offset < PACKAGE_BYTES) {
		size_t length = PACKAGE_BYTES - offset;

		if (length > CHUNK_BYTES)
			length = CHUNK_BYTES;
		memcpy(memory.buffer, package + offset, length);
		memset(&operation, 0, sizeof(operation));
		operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
						      TEEC_NONE, TEEC_NONE, TEEC_NONE);
		operation.params[0].memref.parent = &memory;
		operation.params[0].memref.size = length;
		result = TEEC_InvokeCommand(&session, 0, &operation, &origin);
		printf("invoke offset=%zu offered=%zu result=%08" PRIx32
		       " origin=%" PRIu32 "\n", offset, length, result, origin);
		if (result != TEEC_SUCCESS && result != TEEC_ERROR_BUSY)
			goto out_memory;
		if (offset + length == PACKAGE_BYTES && result != TEEC_SUCCESS) {
			fprintf(stderr, "final chunk returned BUSY, not successful preload\n");
			goto out_memory;
		}
		offset += length;
	}
	printf("Secure-world video package accepted (%zu bytes); decode status is not reported by this service.\n",
	       offset);
	status = 0;
out_memory:
	TEEC_ReleaseSharedMemory(&memory);
out_session:
	TEEC_CloseSession(&session);
out_context:
	TEEC_FinalizeContext(&context);
	return status;
}

static bool child_setup(pid_t parent)
{
	/* A killed supervisor must not leave a privileged provider behind. */
	return !prctl(PR_SET_PDEATHSIG, SIGTERM) && getppid() == parent;
}

static void reap_stopped(pid_t pid)
{
	struct timespec delay = { .tv_nsec = 100000000 };
	int status;
	unsigned int i;

	if (pid <= 0)
		return;
	kill(pid, SIGTERM);
	for (i = 0; i < 50; i++) {
		pid_t result = waitpid(pid, &status, WNOHANG);

		if (result == pid || (result < 0 && errno == ECHILD))
			return;
		nanosleep(&delay, NULL);
	}
	fprintf(stderr, "child %ld did not stop in 5s; sending SIGKILL\n", (long)pid);
	kill(pid, SIGKILL);
	while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
		;
}

int main(int argc, char **argv)
{
	unsigned char *ta = NULL, *package = NULL;
	const char *boot_complete = NULL;
	int first = 1;
	int directory = -1, fd = -1, ready[2] = { -1, -1 };
	pid_t provider_pid = -1, loader_pid = -1, parent = getpid();
	sigset_t signals;
	int status = 1, child_status, sig;
	char byte;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	if (argc == 6 && !strcmp(argv[1], "--boot-complete") && argv[2][0] == '/') {
		boot_complete = argv[2];
		first = 3;
	}
	if (argc - first != 3 || strncmp(argv[first], "/dev/teepriv", 12) ||
	    argv[first][12] < '0' || argv[first][12] > '9' || argv[first][13] ||
	    argv[first + 1][0] != '/' || argv[first + 2][0] != '/') {
		fprintf(stderr, "usage: %s [--boot-complete /ABSOLUTE/SC2_SYSFS_ATTRIBUTE] "
			"/dev/teeprivN /ABSOLUTE/TA_DIRECTORY /ABSOLUTE/video_ucode.bin\n",
			argv[0]);
		return 2;
	}
	/* Never accept block devices, symlinked final components, arbitrary UUIDs,
	 * search directories, environment-driven inputs, or unverified bytes.
	 */
	directory = open(argv[first + 1], O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
	if (directory < 0)
		goto input_error;
	fd = openat(directory, TA_NAME, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
	if (fd < 0)
		goto input_error;
	ta = read_asset(fd, TA_BYTES, ta_sha256);
	close(fd);
	fd = -1;
	close(directory);
	directory = -1;
	if (!ta)
		goto out;
	fd = open(argv[first + 2], O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
	if (fd < 0)
		goto input_error;
	package = read_asset(fd, PACKAGE_BYTES, package_sha256);
	close(fd);
	fd = -1;
	if (!package)
		goto out;
	fprintf(stderr, "SC2 TA and video package hashes verified; starting restricted OP-TEE RPC provider\n");
	/* The same verified byte snapshots are used by both provider processes. */
	if (boot_complete && notify_boot_complete(boot_complete))
		goto out;
	sigemptyset(&signals);
	sigaddset(&signals, SIGCHLD);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGHUP);
	if (sigprocmask(SIG_BLOCK, &signals, NULL) || pipe2(ready, O_CLOEXEC))
		goto input_error;
	provider_pid = fork();
	if (provider_pid < 0)
		goto input_error;
	if (!provider_pid) {
		close(ready[0]);
		if (!child_setup(parent))
			_exit(1);
		_exit(serve(argv[first], ta, ready[1]));
	}
	close(ready[1]);
	ready[1] = -1;
	/* Readiness is emitted only after the privileged device and ABI validate.
	 * SIGINT/TERM are synchronously consumed below; use nonblocking readiness
	 * with sigtimedwait so startup is also interruptible.
	 */
	if (fcntl(ready[0], F_SETFL, O_NONBLOCK))
		goto input_error;
	for (;;) {
		struct timespec interval = { .tv_nsec = 100000000 };
		ssize_t n = read(ready[0], &byte, 1);

		if (n == 1 && byte == 'R')
			break;
		if (!n || (n < 0 && errno != EAGAIN && errno != EINTR))
			goto out;
		sig = sigtimedwait(&signals, NULL, &interval);
		if (sig > 0)
			goto out;
		if (errno != EAGAIN && errno != EINTR)
			goto out;
	}
	close(ready[0]);
	ready[0] = -1;
	loader_pid = fork();
	if (loader_pid < 0)
		goto input_error;
	if (!loader_pid) {
		if (!child_setup(parent))
			_exit(1);
		_exit(preload(package, argv[first]));
	}
	for (;;) {
		pid_t ended = waitpid(-1, &child_status, WNOHANG);

		if (ended == loader_pid) {
			loader_pid = -1;
			if (WIFEXITED(child_status))
				status = WEXITSTATUS(child_status);
			break;
		}
		if (ended == provider_pid) {
			provider_pid = -1;
			fprintf(stderr, "RPC provider exited before preload completed\n");
			break;
		}
		if (ended < 0 && errno != EINTR)
			break;
		if (sigwait(&signals, &sig))
			break;
		if (sig != SIGCHLD) {
			status = 128 + sig;
			break;
		}
	}
	goto out;
input_error:
	perror("preload setup");
out:
	/* Terminate the provider first to release kernel supplicant waiters. */
	if (loader_pid > 0)
		kill(loader_pid, SIGTERM);
	reap_stopped(provider_pid);
	reap_stopped(loader_pid);
	if (fd >= 0)
		close(fd);
	if (directory >= 0)
		close(directory);
	if (ready[0] >= 0)
		close(ready[0]);
	if (ready[1] >= 0)
		close(ready[1]);
	free(package);
	free(ta);
	return status;
}
