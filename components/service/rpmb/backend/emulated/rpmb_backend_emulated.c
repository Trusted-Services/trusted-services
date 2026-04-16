/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rpmb_backend_emulated.h"
#include "util.h"
#include "psa/crypto.h"
#include <stdlib.h>
#include <string.h>

static void u32_to_rpmb_field(uint32_t u32, uint8_t *rpmb_field)
{
	rpmb_field[0] = (u32 >> 24) & 0xff;
	rpmb_field[1] = (u32 >> 16) & 0xff;
	rpmb_field[2] = (u32 >> 8) & 0xff;
	rpmb_field[3] = u32 & 0xff;
}

static uint32_t u32_from_rpmb_field(const uint8_t *rpmb_field)
{
	return (rpmb_field[0] << 24) | (rpmb_field[1] << 16) | (rpmb_field[2] << 8) | rpmb_field[3];
}

static void u16_to_rpmb_field(uint16_t u16, uint8_t *rpmb_field)
{
	rpmb_field[0] = (u16 >> 8) & 0xff;
	rpmb_field[1] = u16 & 0xff;
}

static uint16_t u16_from_rpmb_field(const uint8_t *rpmb_field)
{
	return (rpmb_field[0] << 8) | rpmb_field[1];
}

static psa_status_t
rpmb_backend_emulated_get_dev_info(void *context, uint32_t dev_id, struct rpmb_dev_info *dev_info)
{
	struct rpmb_backend_emulated *backend = (struct rpmb_backend_emulated *)context;
	static const uint8_t test_cid[] = {
		/* MID (Manufacturer ID): Micron */
		0xfe,
		/* CBX (Device/BGA): BGA */
		0x01,
		/* OID (OEM/Application ID) */
		0x4e,
		/* PNM (Product name) "MMC04G" */
		0x4d, 0x4d, 0x43, 0x30, 0x34, 0x47,
		/* PRV (Product revision): 4.2 */
		0x42,
		/* PSN (Product serial number) */
		0xc8, 0xf6, 0x55, 0x2a,
		/*
		 * MDT (Manufacturing date):
		 * June, 2014
		 */
		0x61,
		/* (CRC7 (0xA) << 1) | 0x1 */
		0x15
	};

	(void)dev_id;

	memcpy(dev_info->cid, test_cid, sizeof(dev_info->cid));
	dev_info->rpmb_size_mult = backend->buffer_size / RPMB_SIZE_MULT_UNIT;

	return PSA_SUCCESS;
}

static void calculate_mac(struct rpmb_backend_emulated *backend,
			  const struct rpmb_data_frame *frames, size_t frame_count, uint8_t *mac)
{
	const size_t frame_hash_length =
		sizeof(struct rpmb_data_frame) - offsetof(struct rpmb_data_frame, data);
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
	size_t hash_length = 0;
	size_t i = 0;

	psa_hash_setup(&operation, PSA_ALG_SHA_256);
	psa_hash_update(&operation, backend->key, sizeof(backend->key));

	for (i = 0; i < frame_count; i++) {
		/* Hash data, nonce, write counter, address, block, result, req/resp fields */
		psa_hash_update(&operation, (const uint8_t *)frames[i].data, frame_hash_length);
	}

	psa_hash_finish(&operation, mac, RPMB_KEY_MAC_SIZE, &hash_length);
}

static void rpmb_emulated_authentication_key_write(struct rpmb_backend_emulated *backend,
						   const struct rpmb_data_frame *request,
						   struct rpmb_data_frame *response)
{
	uint16_t result = RPMB_RES_GENERAL_FAILURE;

	memset(response, 0x00, sizeof(*response));

	if (!backend->key_programmed) {
		memcpy(backend->key, request->key_mac, sizeof(backend->key));
		backend->key_programmed = true;
		result = RPMB_RES_OK;
	}

	u16_to_rpmb_field(result, response->op_result);
	u16_to_rpmb_field(RPMB_RESP_TYPE_AUTHENTICATION_KEY_WRITE, response->msg_type);
}

static void rpmb_emulated_read_write_counter(struct rpmb_backend_emulated *backend,
					     const struct rpmb_data_frame *request,
					     struct rpmb_data_frame *response)
{
	uint8_t nonce[RPMB_NONCE_SIZE] = { 0 };

	/*
	 * It is not guarranteed by the RPC layer that the request and response has distinct
	 * memory areas. Thus we need to create a copy of nonce because clearing the response might
	 * clear the request as well.
	 */
	memcpy(nonce, request->nonce, sizeof(nonce));

	memset(response, 0x00, sizeof(*response));
	u16_to_rpmb_field(RPMB_RESP_TYPE_READ_WRITE_COUNTER, response->msg_type);
	u16_to_rpmb_field(RPMB_RES_KEY_NOT_PROGRAMMED, response->op_result);

	if (backend->key_programmed) {
		memcpy(response->nonce, nonce, RPMB_NONCE_SIZE);
		u32_to_rpmb_field(backend->write_counter, response->write_counter);
		u16_to_rpmb_field(RPMB_RES_OK, response->op_result);
		calculate_mac(backend, response, 1, response->key_mac);
	}
}

static uint16_t check_write_request(struct rpmb_backend_emulated *backend,
				    const struct rpmb_data_frame *request)
{
	size_t address = 0;
	uint8_t mac[RPMB_KEY_MAC_SIZE] = { 0 };

	/* Checking as specified in eMMC 6.6.22.4.3 */
	if (backend->write_counter == 0xffffffff)
		return RPMB_RES_COUNTER_EXPIRED | RPMB_RES_WRITE_FAILURE;

	if (MUL_OVERFLOW(u16_from_rpmb_field(request->address), RPMB_DATA_SIZE, &address))
		return RPMB_RES_ADDRESS_FAILURE;

	if (address >= backend->buffer_size)
		return RPMB_RES_ADDRESS_FAILURE;

	/* Only single block write is supported for now */
	if (u16_from_rpmb_field(request->block_count) != 1)
		return RPMB_RES_GENERAL_FAILURE;

	calculate_mac(backend, request, 1, mac);
	if (memcmp(mac, request->key_mac, sizeof(mac)) != 0)
		return RPMB_RES_AUTHENTICATION_FAILURE;

	if (backend->write_counter != u32_from_rpmb_field(request->write_counter))
		return RPMB_RES_COUNTER_FAILURE;

	return RPMB_RES_OK;
}

static void rpmb_emulated_authenticated_data_write(struct rpmb_backend_emulated *backend,
						   const struct rpmb_data_frame *request,
						   struct rpmb_data_frame *response)
{
	uint16_t result = RPMB_RES_KEY_NOT_PROGRAMMED;

	memset(response, 0x00, sizeof(*response));
	u16_to_rpmb_field(RPMB_RESP_TYPE_AUTHENTICATED_DATA_WRITE, response->msg_type);

	if (backend->key_programmed) {
		result = check_write_request(backend, request);
		if (result == RPMB_RES_OK) {
			size_t address = u16_from_rpmb_field(request->address) * RPMB_DATA_SIZE;

			memcpy(&backend->buffer[address], request->data, sizeof(request->data));
			backend->write_counter++;
		}

		memcpy(response->address, request->address, sizeof(response->address));
		u32_to_rpmb_field(backend->write_counter, response->write_counter);
		calculate_mac(backend, response, 1, response->key_mac);
	}

	u16_to_rpmb_field(result, response->op_result);
}

static uint16_t check_read_request(struct rpmb_backend_emulated *backend,
				   const struct rpmb_data_frame *request,
				   size_t *address, size_t *length)
{
	uint16_t block_count = 0;
	size_t end = 0;

	/* Check if start address fits into range */
	if (MUL_OVERFLOW(u16_from_rpmb_field(request->address), RPMB_DATA_SIZE, address))
		return RPMB_RES_ADDRESS_FAILURE;

	if (*address >= backend->buffer_size)
		return RPMB_RES_ADDRESS_FAILURE;

	block_count = u16_from_rpmb_field(request->block_count);
	if (block_count == 0)
		return RPMB_RES_GENERAL_FAILURE;

	/* Check if end address fits into range */
	if (MUL_OVERFLOW(block_count, RPMB_DATA_SIZE, length))
		return RPMB_RES_ADDRESS_FAILURE;

	if (ADD_OVERFLOW(*address, *length, &end))
		return RPMB_RES_ADDRESS_FAILURE;

	if (end > backend->buffer_size)
		return RPMB_RES_ADDRESS_FAILURE;

	return RPMB_RES_OK;
}

static psa_status_t rpmb_emulated_authenticated_data_read(struct rpmb_backend_emulated *backend,
							  const struct rpmb_data_frame *request,
							  struct rpmb_data_frame *response,
							  size_t *response_count)
{
	size_t response_index = 0;

	if (backend->key_programmed) {
		uint16_t result = RPMB_RES_GENERAL_FAILURE;
		size_t address = 0;
		size_t length = 0;

		result = check_read_request(backend, request, &address, &length);
		if (result == RPMB_RES_OK) {
			uint16_t block_count = 0;
			uint16_t i = 0;
			struct rpmb_data_frame temp = { 0 };

			block_count = u16_from_rpmb_field(request->block_count);
			if (block_count > *response_count)
				/*
				 * It is a service level error if available response dataframe count
				 * is less than the requested block count.
				 */
				return PSA_ERROR_BUFFER_TOO_SMALL;

			memcpy(temp.nonce, request->nonce, sizeof(temp.nonce));
			memcpy(temp.address, request->address, sizeof(temp.address));
			memcpy(temp.block_count, request->block_count,
			       sizeof(temp.block_count));
			u16_to_rpmb_field(RPMB_RES_OK, temp.op_result);
			u16_to_rpmb_field(RPMB_RESP_TYPE_AUTHENTICATED_DATA_READ,
					  temp.msg_type);

			for (i = 0; i < block_count; i++) {
				uint8_t *data = &backend->buffer[address + i * RPMB_DATA_SIZE];

				memcpy(&response[i], &temp, sizeof(response[i]));
				memcpy(&response[i].data, data, sizeof(response[i].data));
			}

			calculate_mac(backend, response, block_count,
				      response[block_count - 1].key_mac);
			response_index += block_count;
		}
	} else {
		/* Return single data frame with the error code */
		memset(response, 0x00, sizeof(*response));
		u16_to_rpmb_field(RPMB_RES_KEY_NOT_PROGRAMMED, response->op_result);
		u16_to_rpmb_field(RPMB_RESP_TYPE_AUTHENTICATED_DATA_READ, response->msg_type);
		response_index++;
	}

	*response_count = response_index;

	return PSA_SUCCESS;
}

static psa_status_t
rpmb_backend_emulated_data_request(void *context, uint32_t dev_id,
				   const struct rpmb_data_frame *request_frames,
				   size_t request_frame_count,
				   struct rpmb_data_frame *response_frames,
				   size_t *response_frame_count)
{
	struct rpmb_backend_emulated *backend = (struct rpmb_backend_emulated *)context;
	size_t req_index = 0;
	size_t resp_index = 0;

	if (dev_id != 0)
		return PSA_ERROR_INVALID_ARGUMENT;

	for (req_index = 0; req_index < request_frame_count; req_index++) {
		const struct rpmb_data_frame *request = &request_frames[req_index];

		switch (u16_from_rpmb_field(request->msg_type)) {
		case RPMB_REQ_TYPE_AUTHENTICATION_KEY_WRITE:
			rpmb_emulated_authentication_key_write(backend, request, &backend->result);
			break;

		case RPMB_REQ_TYPE_READ_WRITE_COUNTER:
			if (resp_index < *response_frame_count) {
				struct rpmb_data_frame *response = &response_frames[resp_index++];

				rpmb_emulated_read_write_counter(backend, request, response);
			} else {
				return PSA_ERROR_BUFFER_TOO_SMALL;
			}
			break;

		case RPMB_REQ_TYPE_AUTHENTICATED_DATA_WRITE:
			rpmb_emulated_authenticated_data_write(backend, request, &backend->result);
			break;

		case RPMB_REQ_TYPE_AUTHENTICATED_DATA_READ:
			if (resp_index < *response_frame_count) {
				struct rpmb_data_frame *response = &response_frames[resp_index];
				size_t response_count = *response_frame_count - resp_index;
				psa_status_t status = PSA_ERROR_GENERIC_ERROR;

				status = rpmb_emulated_authenticated_data_read(backend, request,
									       response,
									       &response_count);
				if (status != PSA_SUCCESS)
					return status;

				resp_index += response_count;
			} else {
				return PSA_ERROR_BUFFER_TOO_SMALL;
			}
			break;

		case RPMB_REQ_TYPE_RESULT_READ_REQUEST:
			if (resp_index < *response_frame_count) {
				struct rpmb_data_frame *response = &response_frames[resp_index++];

				memcpy(response, &backend->result, sizeof(*response));
			} else {
				return PSA_ERROR_BUFFER_TOO_SMALL;
			}
			break;

		default:
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	*response_frame_count = resp_index;

	return PSA_SUCCESS;
}

struct rpmb_backend *rpmb_backend_emulated_init(struct rpmb_backend_emulated *context,
						uint8_t size_mult)
{
	static const struct rpmb_backend_interface interface = {
		rpmb_backend_emulated_get_dev_info,
		rpmb_backend_emulated_data_request,
	};

	if (!context || !size_mult)
		return NULL;

	context->buffer_size = size_mult * RPMB_SIZE_MULT_UNIT;
	context->buffer = calloc(1, context->buffer_size);
	if (!context->buffer)
		return NULL;

	context->backend.context = context;
	context->backend.interface = &interface;

	return &context->backend;
}

void rpmb_backend_emulated_deinit(struct rpmb_backend_emulated *context)
{
	free(context->buffer);
}
