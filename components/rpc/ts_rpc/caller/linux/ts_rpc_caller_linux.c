/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ts_rpc_caller_linux.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/tee.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <util.h>

#define INVALID_SESS_ID		  0
#define MAX_TEE_DEV_NUM		  16
#define TS_TEE_DRV_INVALID_SHM_ID (0)

/*
 * This define is part of linux/tee.h starting from Linux v6.10
 * Let's keep a copy here in case the kernel headers come from an older version
 */
#ifndef TEE_IMPL_ID_TSTEE
#define TEE_IMPL_ID_TSTEE 3
#endif

struct ts_tee_dev {
	uint16_t endpoint_id;
	char path[16];
};

struct ts_rpc_caller_linux_context {
	struct ts_tee_dev ts_tee_devs[MAX_TEE_DEV_NUM];
	uint32_t session_id;
	int fd;
};

#define TEE_IOC_OPEN_SESSION_NUM_PARAMS 0
static rpc_status_t open_session(void *context, const struct rpc_uuid *service_uuid,
				 uint16_t endpoint_id)
{
	struct ts_rpc_caller_linux_context *caller = (struct ts_rpc_caller_linux_context *)context;
	const size_t arg_size = sizeof(struct tee_ioctl_open_session_arg) +
				TEE_IOC_OPEN_SESSION_NUM_PARAMS * sizeof(struct tee_ioctl_param);
	union {
		struct tee_ioctl_open_session_arg arg;
		uint8_t data[arg_size];
	} buf;
	struct tee_ioctl_open_session_arg *arg = NULL;
	struct tee_ioctl_buf_data buf_data = { 0 };
	struct ts_tee_dev *dev = NULL;
	int rc = -1;

	if (caller->fd >= 0 || caller->session_id != INVALID_SESS_ID) {
		printf("%s():%d session is already opened\n", __func__, __LINE__);
		return RPC_ERROR_INVALID_STATE;
	}

	for (int i = 0; i < ARRAY_SIZE(caller->ts_tee_devs); i++) {
		if (caller->ts_tee_devs[i].endpoint_id == endpoint_id) {
			dev = &caller->ts_tee_devs[i];
			break;
		}
	}

	if (!dev) {
		printf("%s():%d cannot find device for 0x%04x\n", __func__, __LINE__, endpoint_id);
		return RPC_ERROR_NOT_FOUND;
	}

	caller->fd = open(dev->path, O_RDWR);
	if (caller->fd < 0) {
		printf("%s():%d cannot open %s: %d\n", __func__, __LINE__, dev->path, errno);
		return RPC_ERROR_INTERNAL;
	}

	memset(&buf, 0, sizeof(buf));

	buf_data.buf_ptr = (uintptr_t)&buf;
	buf_data.buf_len = sizeof(buf);

	arg = &buf.arg;
	arg->num_params = TEE_IOC_OPEN_SESSION_NUM_PARAMS;

	memcpy(arg->uuid, service_uuid->uuid, sizeof(service_uuid->uuid));

	rc = ioctl(caller->fd, TEE_IOC_OPEN_SESSION, &buf_data);
	if (rc) {
		close(caller->fd);
		caller->fd = -1;
		return RPC_ERROR_INTERNAL;
	}

	caller->session_id = arg->session;

	return RPC_SUCCESS;
}

static rpc_status_t find_and_open_session(void *context, const struct rpc_uuid *service_uuid)
{
	struct ts_rpc_caller_linux_context *caller = (struct ts_rpc_caller_linux_context *)context;

	for (int i = 0; i < ARRAY_SIZE(caller->ts_tee_devs); i++) {
		if (!open_session(context, service_uuid, caller->ts_tee_devs[i].endpoint_id))
			return RPC_SUCCESS;
	}

	return RPC_ERROR_INTERNAL;
}

rpc_status_t close_session(void *context)
{
	struct ts_rpc_caller_linux_context *caller = (struct ts_rpc_caller_linux_context *)context;
	struct tee_ioctl_close_session_arg arg = { 0 };
	int rc = -1;

	if (caller->fd < 0) {
		printf("%s():%d session is already closed\n", __func__, __LINE__);
		return RPC_ERROR_INVALID_STATE;
	}

	arg.session = caller->session_id;

	rc = ioctl(caller->fd, TEE_IOC_CLOSE_SESSION, &arg);
	if (rc) {
		printf("%s():%d failed to close session: %d\n", __func__, __LINE__, errno);
		return RPC_ERROR_INTERNAL;
	}

	close(caller->fd);
	caller->fd = -1;
	caller->session_id = INVALID_SESS_ID;

	return RPC_SUCCESS;
}

rpc_status_t create_shared_memory(void *context, size_t size,
				  struct rpc_caller_shared_memory *shared_memory)
{
	struct ts_rpc_caller_linux_context *caller = (struct ts_rpc_caller_linux_context *)context;
	struct tee_ioctl_shm_alloc_data data = { .size = size };
	int shm_fd = -1;

	if (!size) {
		shared_memory->buffer = NULL;
		shared_memory->size = 0;
		shared_memory->id = TS_TEE_DRV_INVALID_SHM_ID;

		return RPC_SUCCESS;
	}

	shm_fd = ioctl(caller->fd, TEE_IOC_SHM_ALLOC, &data);
	if (shm_fd < 0) {
		printf("%s():%d failed to create shared memory: %d\n", __func__, __LINE__, errno);
		return RPC_ERROR_INTERNAL;
	}

	shared_memory->buffer =
		mmap(NULL, data.size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shared_memory->buffer == (void *)MAP_FAILED) {
		printf("%s():%d failed to map shared memory: %d\n", __func__, __LINE__, errno);
		close(shm_fd);
		return RPC_ERROR_INTERNAL;
	}
	close(shm_fd);
	shared_memory->size = data.size;
	shared_memory->id = data.id;

	return RPC_SUCCESS;
}

rpc_status_t release_shared_memory(void *context, struct rpc_caller_shared_memory *shared_memory)
{
	(void)context;

	if (shared_memory->id == TS_TEE_DRV_INVALID_SHM_ID)
		return RPC_SUCCESS;

	if (munmap(shared_memory->buffer, shared_memory->size)) {
		printf("%s():%d failed to unmap shared memory: %d\n", __func__, __LINE__, errno);
		return RPC_ERROR_INTERNAL;
	}

	*shared_memory = (struct rpc_caller_shared_memory){ 0 };

	return RPC_SUCCESS;
}

#define TEE_IOC_INVOKE_NUM_PARAMS 1
static rpc_status_t call(void *context, uint16_t opcode,
			 struct rpc_caller_shared_memory *shared_memory, size_t request_length,
			 size_t *response_length, service_status_t *service_status)
{
	struct ts_rpc_caller_linux_context *caller = (struct ts_rpc_caller_linux_context *)context;
	const size_t arg_size = sizeof(struct tee_ioctl_invoke_arg) +
				TEE_IOC_INVOKE_NUM_PARAMS * sizeof(struct tee_ioctl_param);
	union {
		struct tee_ioctl_invoke_arg arg;
		uint8_t data[arg_size];
	} buf;
	struct tee_ioctl_buf_data buf_data = { 0 };
	struct tee_ioctl_invoke_arg *arg = NULL;
	struct tee_ioctl_param *params = NULL;
	int rc = -1;

	memset(&buf, 0, sizeof(buf));

	buf_data.buf_ptr = (uintptr_t)&buf;
	buf_data.buf_len = sizeof(buf);

	arg = &buf.arg;
	arg->func = opcode;
	arg->session = caller->session_id;
	arg->num_params = TEE_IOC_INVOKE_NUM_PARAMS;
	params = (struct tee_ioctl_param *)(arg + 1);

	params[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	params[0].a = shared_memory->id;
	params[0].b = request_length;
	params[0].c = 0;

	rc = ioctl(caller->fd, TEE_IOC_INVOKE, &buf_data);
	if (rc) {
		printf("%s():%d failed to invoke command: %d\n", __func__, __LINE__, errno);
		return RPC_ERROR_INTERNAL;
	}

	*response_length = params[0].a;
	*service_status = (int)arg->ret;

	return RPC_SUCCESS;
}

static void ts_tee_drv_discover(struct ts_tee_dev *ts_tee_devs, size_t count)
{
	struct tee_ioctl_version_data vers = { 0 };
	unsigned int tee_file_index = 0;
	unsigned int ts_tee_dev_index = 0;
	char path[16];
	int rc = -1;
	int fd = -1;

	for (tee_file_index = 0; tee_file_index < MAX_TEE_DEV_NUM && ts_tee_dev_index < count;
	     tee_file_index++) {
		snprintf(path, sizeof(path), "/dev/tee%u", tee_file_index);

		fd = open(path, O_RDWR);
		if (fd < 0)
			continue;

		memset(&vers, 0, sizeof(vers));

		rc = ioctl(fd, TEE_IOC_VERSION, &vers);
		close(fd);

		if (!rc && vers.impl_id == TEE_IMPL_ID_TSTEE) {
			ts_tee_devs[ts_tee_dev_index].endpoint_id = vers.impl_caps;
			memcpy(ts_tee_devs[ts_tee_dev_index].path, path, sizeof(path));
			ts_tee_dev_index++;
		}
	}
}

rpc_status_t ts_rpc_caller_linux_init(struct rpc_caller_interface *rpc_caller)
{
	struct ts_rpc_caller_linux_context *context = NULL;

	if (!rpc_caller || rpc_caller->context)
		return RPC_ERROR_INVALID_VALUE;

	context = (struct ts_rpc_caller_linux_context *)calloc(
		1, sizeof(struct ts_rpc_caller_linux_context));
	if (!context)
		return RPC_ERROR_INTERNAL;

	context->fd = -1;
	context->session_id = INVALID_SESS_ID;

	rpc_caller->context = context;
	rpc_caller->open_session = open_session;
	rpc_caller->find_and_open_session = find_and_open_session;
	rpc_caller->close_session = close_session;
	rpc_caller->create_shared_memory = create_shared_memory;
	rpc_caller->release_shared_memory = release_shared_memory;
	rpc_caller->call = call;

	ts_tee_drv_discover(context->ts_tee_devs, ARRAY_SIZE(context->ts_tee_devs));

	return RPC_SUCCESS;
}

rpc_status_t ts_rpc_caller_linux_deinit(struct rpc_caller_interface *rpc_caller)
{
	struct ts_rpc_caller_linux_context *caller = NULL;

	if (!rpc_caller || !rpc_caller->context)
		return RPC_ERROR_INVALID_VALUE;

	caller = (struct ts_rpc_caller_linux_context *)rpc_caller->context;

	if (caller->session_id != INVALID_SESS_ID) {
		close_session(rpc_caller);
		caller->session_id = INVALID_SESS_ID;
	}

	if (caller->fd >= 0) {
		close(caller->fd);
		caller->fd = -1;
	}

	free(rpc_caller->context);

	return RPC_SUCCESS;
}
