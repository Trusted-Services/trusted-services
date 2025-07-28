/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "http_caller.h"

#include <assert.h>
#include <curl/curl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocols/rpc/common/packed-c/header.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "trace.h"

#define USER_AGENT "libcurl-agent/1.0"

struct payload_buffer {
	uint8_t *data;
	size_t size;
	size_t pos;
};

static rpc_call_handle call_begin(void *context, uint8_t **req_buf, size_t req_len);
static rpc_status_t call_invoke(void *context, rpc_call_handle handle, uint32_t opcode,
				rpc_opstatus_t *opstatus, uint8_t **resp_buf, size_t *resp_len);
static void call_end(void *context, rpc_call_handle handle);

static size_t request_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t bytes_requested = size * nmemb;
	struct payload_buffer *buf = (struct payload_buffer *)userdata;

	size_t bytes_remaining = buf->size - buf->pos;
	size_t bytes_to_send = (bytes_remaining < bytes_requested) ? bytes_remaining :
								     bytes_requested;

	memcpy(ptr, &buf->data[buf->pos], bytes_to_send);

	buf->pos += bytes_to_send;

	return bytes_to_send;
}

static size_t response_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t bytes_received = size * nmemb;
	struct payload_buffer *buf = (struct payload_buffer *)userdata;

	void *data = realloc(buf->data, buf->size + bytes_received);
	if (data != NULL) {
		buf->data = data;
		memcpy(&buf->data[buf->size], ptr, bytes_received);
		buf->size += bytes_received;
	} else {
		EMSG("Out of memory");
		bytes_received = 0;
	}

	return bytes_received;
}

static bool send_head_request(const char *url, struct payload_buffer *response_buf, long *http_code)
{
	assert(url);
	assert(response_buf);

	bool is_success = false;
	CURL *curl_session = curl_easy_init();

	*http_code = 0;

	if (curl_session) {
		curl_easy_setopt(curl_session, CURLOPT_URL, url);
		curl_easy_setopt(curl_session, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(curl_session, CURLOPT_WRITEFUNCTION, response_callback);
		curl_easy_setopt(curl_session, CURLOPT_WRITEDATA, (void *)response_buf);
		curl_easy_setopt(curl_session, CURLOPT_USERAGENT, USER_AGENT);

		CURLcode status = curl_easy_perform(curl_session);

		if (status == CURLE_OK) {
			status = curl_easy_getinfo(curl_session, CURLINFO_RESPONSE_CODE, http_code);
			is_success = (status == CURLE_OK) && (*http_code >= 200) &&
				     (*http_code < 300);
		}

		curl_easy_cleanup(curl_session);
	} else {
		EMSG("Failed to init Curl session");
	}

	return is_success;
}

static bool send_put_request(const char *url, struct payload_buffer *request_buf,
			     struct payload_buffer *response_buf)
{
	bool is_success = false;

	assert(url);
	assert(request_buf);
	assert(response_buf);

	CURL *curl_session = curl_easy_init();

	if (curl_session) {
		curl_easy_setopt(curl_session, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl_session, CURLOPT_URL, url);
		curl_easy_setopt(curl_session, CURLOPT_READFUNCTION, request_callback);
		curl_easy_setopt(curl_session, CURLOPT_READDATA, (void *)request_buf);
		curl_easy_setopt(curl_session, CURLOPT_INFILESIZE_LARGE,
				 (curl_off_t)request_buf->size);
		curl_easy_setopt(curl_session, CURLOPT_WRITEFUNCTION, response_callback);
		curl_easy_setopt(curl_session, CURLOPT_WRITEDATA, (void *)response_buf);
		curl_easy_setopt(curl_session, CURLOPT_USERAGENT, USER_AGENT);

		CURLcode status = curl_easy_perform(curl_session);

		if (status == CURLE_OK) {
			long http_code = 0;

			status =
				curl_easy_getinfo(curl_session, CURLINFO_RESPONSE_CODE, &http_code);
			is_success = (status == CURLE_OK) && (http_code >= 200) &&
				     (http_code < 300);
		}

		curl_easy_cleanup(curl_session);
	} else {
		EMSG("Failed to init Curl session");
	}

	return is_success;
}

static void prepare_call_url(const struct http_caller *s, unsigned int opcode, char *url_buf,
			     size_t url_buf_size)
{
	size_t base_url_len = strnlen(s->rpc_call_url, HTTP_CALLER_MAX_URL_LEN);

	assert(base_url_len > 0);
	assert(base_url_len < url_buf_size);

	memset(url_buf, 0, url_buf_size);
	memcpy(url_buf, s->rpc_call_url, base_url_len);

	/* Ensure '/' is present before adding opcode */
	if (url_buf[base_url_len - 1] != '/') {
		url_buf[base_url_len] = '/';
		base_url_len += 1;
	}

	size_t remaining_space = url_buf_size - base_url_len;
	size_t opcode_len = snprintf(&url_buf[base_url_len], remaining_space, "%u", opcode);

	assert(opcode_len < remaining_space);
}

struct rpc_caller *http_caller_init(struct http_caller *s)
{
	assert(s);

	struct rpc_caller *base = &s->rpc_caller;

	rpc_caller_init(base, s);
	base->call_begin = call_begin;
	base->call_invoke = call_invoke;
	base->call_end = call_end;

	memset(s->rpc_call_url, 0, sizeof(s->rpc_call_url));

	s->req_body_size = 0;
	s->req_body_buf = NULL;
	s->resp_body_buf = NULL;

	CURLcode status = curl_global_init(CURL_GLOBAL_ALL);

	return (status == CURLE_OK) ? base : NULL;
}

void http_caller_deinit(struct http_caller *s)
{
	assert(s);

	s->rpc_caller.context = NULL;
	s->rpc_caller.call_begin = NULL;
	s->rpc_caller.call_invoke = NULL;
	s->rpc_caller.call_end = NULL;

	call_end(s, s);
}

bool http_caller_probe(const char *url, long *http_code)
{
	assert(url);

	struct payload_buffer response_buf;

	response_buf.data = NULL;
	response_buf.size = 0;

	bool is_reached = send_head_request(url, &response_buf, http_code);

	free(response_buf.data);

	return is_reached;
}

int http_caller_open(struct http_caller *s, const char *rpc_call_url)
{
	assert(s);
	assert(rpc_call_url);

	strncpy(s->rpc_call_url, rpc_call_url, sizeof(s->rpc_call_url));

	return 0;
}

int http_caller_close(struct http_caller *s)
{
	(void)s;
	return 0;
}

static rpc_call_handle call_begin(void *context, uint8_t **req_buf, size_t req_len)
{
	assert(context);
	assert(req_buf || !req_len);

	rpc_call_handle handle = NULL;
	struct http_caller *s = (struct http_caller *)context;

	if (!s->req_body_buf) {
		size_t req_body_size = sizeof(struct ts_rpc_req_hdr) + req_len;

		s->req_body_buf = malloc(req_body_size);

		if (s->req_body_buf) {
			memset(s->req_body_buf, 0, req_body_size);

			handle = s;
			*req_buf = &s->req_body_buf[sizeof(struct ts_rpc_req_hdr)];
			s->req_body_size = req_body_size;

			struct ts_rpc_req_hdr *rpc_hdr = (struct ts_rpc_req_hdr *)s->req_body_buf;

			rpc_hdr->encoding = s->rpc_caller.encoding;
			rpc_hdr->param_len = req_len;

		} else {
			EMSG("Out of memory");
		}
	}

	return handle;
}

static rpc_status_t call_invoke(void *context, const rpc_call_handle handle, uint32_t opcode,
				rpc_opstatus_t *opstatus, uint8_t **resp_buf, size_t *resp_len)
{
	rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
	struct http_caller *s = (struct http_caller *)context;

	*resp_len = 0;

	if ((handle == s) && s->req_body_buf) {
		struct ts_rpc_req_hdr *rpc_hdr = (struct ts_rpc_req_hdr *)s->req_body_buf;
		struct payload_buffer request_buf = { 0 };
		struct payload_buffer response_buf = { 0 };

		rpc_status = TS_RPC_ERROR_EP_DOES_NOT_EXIT;

		rpc_hdr->opcode = opcode;

		request_buf.data = s->req_body_buf;
		request_buf.size = s->req_body_size;

		char call_url[HTTP_CALLER_MAX_URL_LEN];

		prepare_call_url(s, opcode, call_url, sizeof(call_url));

		if (send_put_request(call_url, &request_buf, &response_buf) && response_buf.data &&
		    response_buf.size >= sizeof(struct ts_rpc_resp_hdr)) {
			struct ts_rpc_resp_hdr *resp_hdr =
				(struct ts_rpc_resp_hdr *)response_buf.data;
			size_t response_param_len =
				response_buf.size - sizeof(struct ts_rpc_resp_hdr);

			if (resp_hdr->param_len == response_param_len) {
				rpc_status = resp_hdr->rpc_status;

				if (rpc_status == TS_RPC_CALL_ACCEPTED) {
					*resp_buf =
						&response_buf.data[sizeof(struct ts_rpc_resp_hdr)];
					*resp_len = response_param_len;

					*opstatus = resp_hdr->op_status;
				}

			} else {
				rpc_status = TS_RPC_ERROR_INVALID_RESP_BODY;
			}
		}
	}

	return rpc_status;
}

static void call_end(void *context, const rpc_call_handle handle)
{
	struct http_caller *s = (struct http_caller *)context;

	if ((handle == s) && s->req_body_buf) {
		free(s->req_body_buf);
		s->req_body_buf = NULL;

		free(s->resp_body_buf);
		s->resp_body_buf = NULL;
	}
}
