// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 */

#include <CppUTestExt/MockSupport.h>
#include "mock_ffa_api.h"

void expect_ffa_version(const uint32_t *version, ffa_result result)
{
	mock().expectOneCall("ffa_version")
		.withOutputParameterReturning("version", version,
					      sizeof(*version))
		.andReturnValue(result);
}

ffa_result ffa_version(uint32_t *version)
{
	return mock()
		.actualCall("ffa_version")
		.withOutputParameter("version", version)
		.returnIntValue();
}

void expect_ffa_features(
	uint32_t ffa_function_id,
	const struct ffa_interface_properties *interface_properties,
	ffa_result result)
{
	mock().expectOneCall("ffa_features")
		.withOutputParameterReturning("interface_properties",
					      interface_properties,
					      sizeof(*interface_properties))
		.andReturnValue(result);
}

ffa_result ffa_features(uint32_t ffa_function_id,
			struct ffa_interface_properties *interface_properties)
{
	return mock()
		.actualCall("ffa_features")
		.withOutputParameter("interface_properties",
				     interface_properties)
		.returnIntValue();
}

void expect_ffa_rx_release(ffa_result result)
{
	mock().expectOneCall("ffa_rx_telease").andReturnValue(result);
}

ffa_result ffa_rx_release(void)
{
	return mock().actualCall("ffa_rx_telease").returnIntValue();
}

void expect_ffa_rxtx_map(const void *tx_buffer, const void *rx_buffer,
			 uint32_t page_count, ffa_result result)
{
	mock().expectOneCall("ffa_rxtx_map")
		.withConstPointerParameter("tx_buffer", tx_buffer)
		.withConstPointerParameter("rx_buffer", rx_buffer)
		.withUnsignedIntParameter("page_count", page_count)
		.andReturnValue(result);
}

ffa_result ffa_rxtx_map(const void *tx_buffer, const void *rx_buffer,
			uint32_t page_count)
{
	return mock()
		.actualCall("ffa_rxtx_map")
		.withConstPointerParameter("tx_buffer", tx_buffer)
		.withConstPointerParameter("rx_buffer", rx_buffer)
		.withUnsignedIntParameter("page_count", page_count)
		.returnIntValue();
}

void expect_ffa_rxtx_unmap(uint16_t id, ffa_result result)
{
	mock().expectOneCall("ffa_rxtx_unmap")
		.withUnsignedIntParameter("id", id)
		.andReturnValue(result);
}

ffa_result ffa_rxtx_unmap(uint16_t id)
{
	return mock()
		.actualCall("ffa_rxtx_unmap")
		.withUnsignedIntParameter("id", id)
		.returnIntValue();
}

#if CFG_FFA_VERSION == FFA_VERSION_1_0
void expect_ffa_partition_info_get(const struct ffa_uuid *uuid,
				   const uint32_t *count, ffa_result result)
{
	mock().expectOneCall("ffa_partition_info_get")
		.withMemoryBufferParameter("uuid", (const unsigned char *)uuid,
					   sizeof(*uuid))
		.withOutputParameterReturning("count", count, sizeof(*count))
		.andReturnValue(result);
}

ffa_result ffa_partition_info_get(const struct ffa_uuid *uuid, uint32_t *count)
{
	return mock()
		.actualCall("ffa_partition_info_get")
		.withMemoryBufferParameter("uuid", (const unsigned char *)uuid,
					   sizeof(*uuid))
		.withOutputParameter("count", count)
		.returnIntValue();
}
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
void expect_ffa_partition_info_get(const struct ffa_uuid *uuid, uint32_t flags,
				   const uint32_t *count, const uint32_t *size,
				   ffa_result result)
{
	mock().expectOneCall("ffa_partition_info_get")
		.withMemoryBufferParameter("uuid", (const unsigned char *)uuid, sizeof(*uuid))
		.withUnsignedIntParameter("flags", flags)
		.withOutputParameterReturning("count", count, sizeof(*count))
		.withOutputParameterReturning("size", size, sizeof(*size))
		.andReturnValue(result);
}

ffa_result ffa_partition_info_get(const struct ffa_uuid *uuid, uint32_t flags, uint32_t *count,
				  uint32_t *size)
{
	return mock()
		.actualCall("ffa_partition_info_get")
		.withMemoryBufferParameter("uuid", (const unsigned char *)uuid, sizeof(*uuid))
		.withUnsignedIntParameter("flags", flags)
		.withOutputParameter("count", count)
		.withOutputParameter("size", size)
		.returnIntValue();
}
#endif /* CFG_FFA_VERSION */

void expect_ffa_id_get(const uint16_t *id, ffa_result result)
{
	mock().expectOneCall("ffa_id_get")
		.withOutputParameterReturning("id", id, sizeof(*id))
		.andReturnValue(result);
}

ffa_result ffa_id_get(uint16_t *id)
{
	return mock()
		.actualCall("ffa_id_get")
		.withOutputParameter("id", id)
		.returnIntValue();
}

void expect_ffa_msg_wait(const struct ffa_direct_msg *msg, ffa_result result)
{
	mock().expectOneCall("ffa_msg_wait")
		.withOutputParameterReturning("msg", msg, sizeof(*msg))
		.andReturnValue(result);
}

ffa_result ffa_msg_wait(struct ffa_direct_msg *msg)
{
	return mock()
		.actualCall("ffa_msg_wait")
		.withOutputParameter("msg", msg)
		.returnIntValue();
}

void expect_ffa_msg_send_direct_req_32(uint16_t source, uint16_t dest,
				       uint32_t a0, uint32_t a1, uint32_t a2,
				       uint32_t a3, uint32_t a4,
				       const struct ffa_direct_msg *msg,
				       ffa_result result)
{
	mock().expectOneCall("ffa_msg_send_direct_req_32")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedIntParameter("a0", a0)
		.withUnsignedIntParameter("a1", a1)
		.withUnsignedIntParameter("a2", a2)
		.withUnsignedIntParameter("a3", a3)
		.withUnsignedIntParameter("a4", a4)
		.withOutputParameterReturning("msg", msg, sizeof(*msg))
		.andReturnValue(result);
}

ffa_result ffa_msg_send_direct_req_32(uint16_t source, uint16_t dest,
				      uint32_t a0, uint32_t a1, uint32_t a2,
				      uint32_t a3,uint32_t a4,
				      struct ffa_direct_msg *msg)
{
	return mock()
		.actualCall("ffa_msg_send_direct_req_32")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedIntParameter("a0", a0)
		.withUnsignedIntParameter("a1", a1)
		.withUnsignedIntParameter("a2", a2)
		.withUnsignedIntParameter("a3", a3)
		.withUnsignedIntParameter("a4", a4)
		.withOutputParameter("msg", msg)
		.returnIntValue();
}

void expect_ffa_msg_send_direct_req_64(uint16_t source, uint16_t dest,
				       uint64_t a0, uint64_t a1, uint64_t a2,
				       uint64_t a3, uint64_t a4,
				       const struct ffa_direct_msg *msg,
				       ffa_result result)
{
	mock().expectOneCall("ffa_msg_send_direct_req_64")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedLongIntParameter("a0", a0)
		.withUnsignedLongIntParameter("a1", a1)
		.withUnsignedLongIntParameter("a2", a2)
		.withUnsignedLongIntParameter("a3", a3)
		.withUnsignedLongIntParameter("a4", a4)
		.withOutputParameterReturning("msg", msg, sizeof(*msg))
		.andReturnValue(result);
}

ffa_result ffa_msg_send_direct_req_64(uint16_t source, uint16_t dest,
				      uint64_t a0, uint64_t a1, uint64_t a2,
				      uint64_t a3, uint64_t a4,
				      struct ffa_direct_msg *msg)
{
	return mock()
		.actualCall("ffa_msg_send_direct_req_64")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedLongIntParameter("a0", a0)
		.withUnsignedLongIntParameter("a1", a1)
		.withUnsignedLongIntParameter("a2", a2)
		.withUnsignedLongIntParameter("a3", a3)
		.withUnsignedLongIntParameter("a4", a4)
		.withOutputParameter("msg", msg)
		.returnIntValue();
}

void expect_ffa_msg_send_direct_resp_32(uint16_t source, uint16_t dest,
				     uint32_t a0, uint32_t a1, uint32_t a2,
				     uint32_t a3, uint32_t a4,
				     const struct ffa_direct_msg *msg,
				     ffa_result result)
{
	mock().expectOneCall("ffa_msg_send_direct_resp_32")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedIntParameter("a0", a0)
		.withUnsignedIntParameter("a1", a1)
		.withUnsignedIntParameter("a2", a2)
		.withUnsignedIntParameter("a3", a3)
		.withUnsignedIntParameter("a4", a4)
		.withOutputParameterReturning("msg", msg, sizeof(*msg))
		.andReturnValue(result);
}

ffa_result ffa_msg_send_direct_resp_32(uint16_t source, uint16_t dest, uint32_t a0,
				    uint32_t a1, uint32_t a2, uint32_t a3,
				    uint32_t a4, struct ffa_direct_msg *msg)
{
	return mock()
		.actualCall("ffa_msg_send_direct_resp_32")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedIntParameter("a0", a0)
		.withUnsignedIntParameter("a1", a1)
		.withUnsignedIntParameter("a2", a2)
		.withUnsignedIntParameter("a3", a3)
		.withUnsignedIntParameter("a4", a4)
		.withOutputParameter("msg", msg)
		.returnIntValue();
}

void expect_ffa_msg_send_direct_resp_64(uint16_t source, uint16_t dest,
				     uint64_t a0, uint64_t a1, uint64_t a2,
				     uint64_t a3, uint64_t a4,
				     const struct ffa_direct_msg *msg,
				     ffa_result result)
{
	mock().expectOneCall("ffa_msg_send_direct_resp_64")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedLongIntParameter("a0", a0)
		.withUnsignedLongIntParameter("a1", a1)
		.withUnsignedLongIntParameter("a2", a2)
		.withUnsignedLongIntParameter("a3", a3)
		.withUnsignedLongIntParameter("a4", a4)
		.withOutputParameterReturning("msg", msg, sizeof(*msg))
		.andReturnValue(result);
}

ffa_result ffa_msg_send_direct_resp_64(uint16_t source, uint16_t dest, uint64_t a0,
				    uint64_t a1, uint64_t a2, uint64_t a3,
				    uint64_t a4, struct ffa_direct_msg *msg)
{
	return mock()
		.actualCall("ffa_msg_send_direct_resp_64")
		.withUnsignedIntParameter("source", source)
		.withUnsignedIntParameter("dest", dest)
		.withUnsignedLongIntParameter("a0", a0)
		.withUnsignedLongIntParameter("a1", a1)
		.withUnsignedLongIntParameter("a2", a2)
		.withUnsignedLongIntParameter("a3", a3)
		.withUnsignedLongIntParameter("a4", a4)
		.withOutputParameter("msg", msg)
		.returnIntValue();
}

void expect_ffa_mem_donate(uint32_t total_length, uint32_t fragment_length,
			   void *buffer_address, uint32_t page_count,
			   const uint64_t *handle, ffa_result result)
{
	mock().expectOneCall("ffa_mem_donate")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameterReturning("handle", handle, sizeof(*handle))
		.andReturnValue(result);
}

ffa_result ffa_mem_donate(uint32_t total_length, uint32_t fragment_length,
			  void *buffer_address, uint32_t page_count,
			  uint64_t *handle)
{
	return mock()
		.actualCall("ffa_mem_donate")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameter("handle", handle)
		.returnIntValue();
}

void expect_ffa_mem_donate_rxtx(uint32_t total_length, uint32_t fragment_length,
				const uint64_t *handle, ffa_result result)
{
	mock().expectOneCall("ffa_mem_donate_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameterReturning("handle", handle, sizeof(*handle))
		.andReturnValue(result);
}

ffa_result ffa_mem_donate_rxtx(uint32_t total_length, uint32_t fragment_length,
			       uint64_t *handle)
{
	return mock()
		.actualCall("ffa_mem_donate_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameter("handle", handle)
		.returnIntValue();
}

void expect_ffa_mem_lend(uint32_t total_length, uint32_t fragment_length,
			 void *buffer_address, uint32_t page_count,
			 const uint64_t *handle, ffa_result result)
{
	mock().expectOneCall("ffa_mem_lend")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameterReturning("handle", handle, sizeof(*handle))
		.andReturnValue(result);
}

ffa_result ffa_mem_lend(uint32_t total_length, uint32_t fragment_length,
			void *buffer_address, uint32_t page_count,
			uint64_t *handle)
{
	return mock()
		.actualCall("ffa_mem_lend")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameter("handle", handle)
		.returnIntValue();
}

void expect_ffa_mem_lend_rxtx(uint32_t total_length, uint32_t fragment_length,
			      const uint64_t *handle, ffa_result result)
{
	mock().expectOneCall("ffa_mem_lend_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameterReturning("handle", handle, sizeof(*handle))
		.andReturnValue(result);
}

ffa_result ffa_mem_lend_rxtx(uint32_t total_length, uint32_t fragment_length,
			     uint64_t *handle)
{
	return mock()
		.actualCall("ffa_mem_lend_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameter("handle", handle)
		.returnIntValue();
}

void expect_ffa_mem_share(uint32_t total_length, uint32_t fragment_length,
			  void *buffer_address, uint32_t page_count,
			  const uint64_t *handle, ffa_result result)
{
	mock().expectOneCall("ffa_mem_share")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameterReturning("handle", handle, sizeof(*handle))
		.andReturnValue(result);
}

ffa_result ffa_mem_share(uint32_t total_length, uint32_t fragment_length,
			 void *buffer_address, uint32_t page_count,
			 uint64_t *handle)
{
	return mock()
		.actualCall("ffa_mem_share")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameter("handle", handle)
		.returnIntValue();
}

void expect_ffa_mem_share_rxtx(uint32_t total_length, uint32_t fragment_length,
			       const uint64_t *handle, ffa_result result)
{
	mock().expectOneCall("ffa_mem_share_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameterReturning("handle", handle, sizeof(*handle))
		.andReturnValue(result);
}

ffa_result ffa_mem_share_rxtx(uint32_t total_length, uint32_t fragment_length,
			      uint64_t *handle)
{
	return mock()
		.actualCall("ffa_mem_share_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameter("handle", handle)
		.returnIntValue();
}

void expect_ffa_mem_retrieve_req(uint32_t total_length,
				 uint32_t fragment_length, void *buffer_address,
				 uint32_t page_count,
				 const uint32_t *resp_total_length,
				 const uint32_t *resp_fragment_length,
				 ffa_result result)
{
	mock().expectOneCall("ffa_mem_retrieve_req")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameterReturning("resp_total_length",
					      resp_total_length,
					      sizeof(*resp_total_length))
		.withOutputParameterReturning("resp_fragment_length",
					      resp_fragment_length,
					      sizeof(*resp_fragment_length))
		.andReturnValue(result);
}

ffa_result ffa_mem_retrieve_req(uint32_t total_length, uint32_t fragment_length,
				void *buffer_address, uint32_t page_count,
				uint32_t *resp_total_length,
				uint32_t *resp_fragment_length)
{
	return mock()
		.actualCall("ffa_mem_retrieve_req")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withPointerParameter("buffer_address", buffer_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withOutputParameter("resp_total_length", resp_total_length)
		.withOutputParameter("resp_fragment_length",
				     resp_fragment_length)
		.returnIntValue();
}

void expect_ffa_mem_retrieve_req_rxtx(uint32_t total_length,
				      uint32_t fragment_length,
				      const uint32_t *resp_total_length,
				      const uint32_t *resp_fragment_length,
				      ffa_result result)
{
	mock().expectOneCall("ffa_mem_retrieve_req_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameterReturning("resp_total_length",
					      resp_total_length,
					      sizeof(*resp_total_length))
		.withOutputParameterReturning("resp_fragment_length",
					      resp_fragment_length,
					      sizeof(*resp_fragment_length))
		.andReturnValue(result);
}

ffa_result ffa_mem_retrieve_req_rxtx(uint32_t total_length,
				     uint32_t fragment_length,
				     uint32_t *resp_total_length,
				     uint32_t *resp_fragment_length)
{
	return mock()
		.actualCall("ffa_mem_retrieve_req_rxtx")
		.withUnsignedIntParameter("total_length", total_length)
		.withUnsignedIntParameter("fragment_length", fragment_length)
		.withOutputParameter("resp_total_length", resp_total_length)
		.withOutputParameter("resp_fragment_length",
				     resp_fragment_length)
		.returnIntValue();
}

void expect_ffa_mem_relinquish(ffa_result result)
{
	mock().expectOneCall("ffa_mem_relinquish").andReturnValue(result);
}

ffa_result ffa_mem_relinquish(void)
{
	return mock().actualCall("ffa_mem_relinquish").returnIntValue();
}

void expect_ffa_mem_reclaim(uint64_t handle, uint32_t flags, ffa_result result)
{
	mock().expectOneCall("ffa_mem_reclaim")
		.withUnsignedLongIntParameter("handle", handle)
		.withUnsignedIntParameter("flags", flags)
		.andReturnValue(result);
}

ffa_result ffa_mem_reclaim(uint64_t handle, uint32_t flags)
{
	return mock()
		.actualCall("ffa_mem_reclaim")
		.withUnsignedLongIntParameter("handle", handle)
		.withUnsignedIntParameter("flags", flags)
		.returnIntValue();
}

void expect_ffa_mem_perm_get(const void *base_address, const uint32_t *mem_perm,
			     ffa_result result)
{
	mock().expectOneCall("ffa_mem_perm_get")
		.withConstPointerParameter("base_address", base_address)
		.withOutputParameterReturning("mem_perm", mem_perm,
					      sizeof(*mem_perm))
		.andReturnValue(result);
}

ffa_result ffa_mem_perm_get(const void *base_address, uint32_t *mem_perm)
{
	return mock().actualCall("ffa_mem_perm_get")
		.withConstPointerParameter("base_address", base_address)
		.withOutputParameter("mem_perm", mem_perm)
		.returnIntValue();
}

void expect_ffa_mem_perm_set(const void *base_address, uint32_t page_count,
			     uint32_t mem_perm, ffa_result result)
{
	mock().expectOneCall("ffa_mem_perm_set")
		.withConstPointerParameter("base_address", base_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withUnsignedIntParameter("mem_perm", mem_perm)
		.andReturnValue(result);
}

ffa_result ffa_mem_perm_set(const void *base_address, uint32_t page_count,
			    uint32_t mem_perm)
{
	return mock().actualCall("ffa_mem_perm_set")
		.withConstPointerParameter("base_address", base_address)
		.withUnsignedIntParameter("page_count", page_count)
		.withUnsignedIntParameter("mem_perm", mem_perm)
		.returnIntValue();
}

void expect_ffa_console_log_32(const char *message, size_t length,
			       ffa_result result)
{
	mock().expectOneCall("ffa_console_log_32")
		.withStringParameter("message", message)
		.withUnsignedIntParameter("length", length)
		.andReturnValue(result);
}

ffa_result ffa_console_log_32(const char *message, size_t length)
{
	return mock()
		.actualCall("ffa_console_log_32")
		.withStringParameter("message", message)
		.withUnsignedIntParameter("length", length)
		.returnIntValue();
}

void expect_ffa_console_log_64(const char *message, size_t length,
			       ffa_result result)
{
	mock().expectOneCall("ffa_console_log_64")
		.withStringParameter("message", message)
		.withUnsignedIntParameter("length", length)
		.andReturnValue(result);
}

ffa_result ffa_console_log_64(const char *message, size_t length)
{
	return mock()
		.actualCall("ffa_console_log_64")
		.withStringParameter("message", message)
		.withUnsignedIntParameter("length", length)
		.returnIntValue();
}
