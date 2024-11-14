/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_INCLUDE_FFA_MEMORY_DESCRIPTORS_H_
#define LIBSP_INCLUDE_FFA_MEMORY_DESCRIPTORS_H_

/**
 * @file  ffa_memory_descriptor.h
 * @brief The functions of this file were made to help building and parsing
 *        memory transaction descriptors.
 */

#include "ffa_api_types.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Helper buffer descriptor type for passing buffer pointer and length
 *        along the current buffer usage.
 */
struct ffa_mem_transaction_buffer {
	void *buffer;
	size_t length;
	size_t used;
};

/**
 * @brief      Fills the buffer descriptor to use the area specified by the
 *             address and length parameters.
 *
 * @param[in]  address  The buffer address
 * @param[in]  length   The buffer length
 * @param[out] buffer   The buffer descriptor
 */
void ffa_init_mem_transaction_buffer(void *address, size_t length,
				     struct ffa_mem_transaction_buffer *buffer);

/**
 * @brief      Initializes the memory transaction descriptor (see Table 5.19) in
 *             the transaction buffer.
 *
 * @param[in]  buffer           The buffer descriptor
 * @param[in]  sender_id        ID of the Owner endpoint
 * @param[in]  mem_region_attr  The memory region attributes
 * @param[in]  flags            The transaction flags
 * @param[in]  handle           Memory region handle
 * @param[in]  tag              Memory region tag
 */
void ffa_init_mem_transaction_desc(struct ffa_mem_transaction_buffer *buffer,
				   uint16_t sender_id, uint16_t mem_region_attr,
				   uint32_t flags, uint64_t handle, uint64_t tag);

/**
 * @brief      Queries the memory transaction descriptor (see Table 5.19) from
 *             the transaction buffer.
 *
 * @param[in]  buffer  The buffer descriptor
 *
 * @return     Pointer to the transaction descriptor.
 */
const struct ffa_mem_transaction_desc *
ffa_get_mem_transaction_desc(struct ffa_mem_transaction_buffer *buffer);

/**
 * @brief      Reserves space for the memory access descriptors. In case of
 *             alternately adding memory access descriptors and adding memory
 *             regions to these descriptors the descriptor add will panic unless
 *             there was a sufficient amount of space reserved by this function.
 *
 * @param[in]  buffer  The buffer descriptor
 * @param[in]  count   Memory access descriptor count to reserve
 */
void ffa_reserve_mem_access_desc(struct ffa_mem_transaction_buffer *buffer,
				 size_t count);

/**
 * @brief      Adds a memory access descriptor (see Table 5.16).
 *
 * @param[in]  buffer           The buffer descriptor
 * @param[in]  endpoint_id      16-bit ID of endpoint to which the memory access
 *                              permissions apply
 * @param[in]  mem_access_perm  Permissions used to access a memory region
 * @param[in]  flags            ABI specific flags
 *
 * @return     Index of the newly added descriptor
 */
uint32_t ffa_add_mem_access_desc(struct ffa_mem_transaction_buffer *buffer,
				 uint16_t endpoint_id, uint8_t mem_access_perm,
				 uint8_t flags);

/**
 * @brief      Queries the count of the memory access descriptors (see Table
 *             5.16) from the transaction buffer.
 *
 * @param[in]  buffer  The buffer descriptor
 *
 * @return     Descriptor count
 */
uint32_t
ffa_get_mem_access_desc_count(struct ffa_mem_transaction_buffer *buffer);

/**
 * @brief      Queries the memory access descriptor (see Table 5.16) from the
 *             transaction buffer.
 *
 * @param[in]  buffer            The buffer descriptor
 * @param[in]  descriptor_index  The descriptor index
 *
 * @return     Pointer to the memory access descriptor
 */
const struct ffa_mem_access_desc *
ffa_get_mem_access_desc(struct ffa_mem_transaction_buffer *buffer,
			uint32_t descriptor_index);

/**
 * @brief      Adds a memory region to the transaction (see Table 5.13 and 5.14)
 *
 * @param[in]  buffer            The buffer descriptor
 * @param[in]  address           The address of the region
 * @param[in]  page_count        The size of the region in 4K pages
 */
void ffa_add_memory_region(struct ffa_mem_transaction_buffer *buffer,
			   const void *address, uint32_t page_count);

/**
 * @brief      Queries the memory region descriptor (see Table 5.13 and 5.14)
 *             from the transaction buffer.
 *
 * @param[in]  buffer            The buffer descriptor
 *
 * @return     Pointer to the memory region descriptor
 */
const struct ffa_composite_mem_region_desc *
ffa_get_memory_region(struct ffa_mem_transaction_buffer *buffer);

#ifdef __cplusplus
}
#endif

#endif /* LIBSP_INCLUDE_FFA_MEMORY_DESCRIPTORS_H_ */
