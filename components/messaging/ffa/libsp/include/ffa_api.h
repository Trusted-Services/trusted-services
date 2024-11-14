/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_INCLUDE_FFA_API_H_
#define LIBSP_INCLUDE_FFA_API_H_

/**
 * @file  ffa_api.h
 * @brief The file contains wrapper functions around the FF-A interfaces
 *        described in sections 7-11 of the specification.
 */

#include "ffa_api_types.h"
#include "ffa_api_defines.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Setup and discovery interfaces
 */

/**
 * @brief      Queries the version of the Firmware Framework implementation at
 *             the FF-A instance.
 *
 * @param[out] version  Version number of the FF-A implementation
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_version(uint32_t *version);

/**
 * @brief      Queries whether the FF-A interface is implemented of the
 *             component at the higher EL and if it implements any optional
 *             features. The meaning of the interface_properties structure
 *             depends on the queried FF-A function and it is described in
 *             section 8.2 of the FF-A standard (v1.0).
 *
 * @param[in]  ffa_function_id       The function id of the queried FF-A
 *                                   function
 * @param[out] interface_properties  Used to encode any optional features
 *                                   implemented or any implementation details
 *                                   exported by the queried interface
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_features(uint32_t ffa_function_id,
			struct ffa_interface_properties *interface_properties);

/**
 * @brief      Relinquishes the ownership of the RX buffer after reading a
 *             message from it.
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_rx_release(void);

/**
 * @brief      Maps the RX/TX buffer pair in the callee's translation regime.
 *
 * @param[in]  tx_buffer   Base address of the TX buffer
 * @param[in]  rx_buffer   Base address of the RX buffer
 * @param[in]  page_count  Number of contiguous 4K pages allocated for each
 *                         buffer
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_rxtx_map(const void *tx_buffer, const void *rx_buffer,
			uint32_t page_count);

/**
 * @brief      Unmaps the RX/TX buffer pair in the callee's translation regime.
 *
 * @param[in]  id     ID of FF-A component that allocated the RX/TX buffer
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_rxtx_unmap(uint16_t id);

#if CFG_FFA_VERSION == FFA_VERSION_1_0
/**
 * @brief      Requests the SPM to return information about the partitions of
 *             the system. Nil UUID can be used to return information about all
 *             the SPs of the system. The information is returned in the RX
 *             buffer of the caller as an array of ffa_partition_information
 *             structures.
 *             This is an FF-A v1.0 call.
 *
 * @param[in]  uuid   The uuid
 * @param[out] count  Count of partition information descriptors populated in
 *                    RX buffer of caller
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_partition_info_get(const struct ffa_uuid *uuid, uint32_t *count);
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1

/**
 * @brief      Requests the SPM to return information about the partitions of
 *             the system. Nil UUID can be used to return information about all
 *             the SPs of the system. The information is returned in the RX
 *             buffer of the caller as an array of ffa_partition_information
 *             structures.
 *             By settings the flags parameter, the function can be instructed
 *             to only return the count of SPs with the matching UUID.
 *             This is an FF-A v1.1 call.
 *
 * @param[in]  uuid     The uuid
 * @param[in]  flags	FFA_PARTITION_INFO_GET_FLAG_* flag value
 * @param[out] count    Count of matching SPs
 * @param[out] size     Size of the partition information descriptors
 * @return ffa_result
 */
ffa_result ffa_partition_info_get(const struct ffa_uuid *uuid, uint32_t flags,
				  uint32_t *count, uint32_t *size);
#endif /* CFG_FFA_VERSION */

/**
 * @brief      Returns the 16 bit ID of the calling FF-A component
 *
 * @param      id    ID of the caller
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_id_get(uint16_t *id);

/**
 * CPU cycle management interfaces
 */

/**
 * @brief      Blocks the caller until a message is available or until an
 *             interrupt happens. It is also used to indicate the completion of
 *             the boot phase and the end of the interrupt handling.
 * @note       The ffa_interrupt_handler function can be called during the
 *             execution of this function.
 *
 * @param[out] msg   The incoming message
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_msg_wait(struct ffa_direct_msg *msg);

/** Messaging interfaces */

/**
 * @brief      Sends a 32 bit partition message in parameter registers as a
 *             request and blocks until the response is available.
 * @note       The ffa_interrupt_handler function can be called during the
 *             execution of this function
 *
 * @param[in]  source            Source endpoint ID
 * @param[in]  dest              Destination endpoint ID
 * @param[in]  a0,a1,a2,a3,a4    Implementation defined message values
 * @param[out] msg               The response message
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_msg_send_direct_req_32(uint16_t source, uint16_t dest,
				      uint32_t a0, uint32_t a1, uint32_t a2,
				      uint32_t a3, uint32_t a4,
				      struct ffa_direct_msg *msg);

/**
 * @brief      Sends a 64 bit partition message in parameter registers as a
 *             request and blocks until the response is available.
 * @note       The ffa_interrupt_handler function can be called during the
 *             execution of this function
 *
 * @param[in]  source            Source endpoint ID
 * @param[in]  dest              Destination endpoint ID
 * @param[in]  a0,a1,a2,a3,a4    Implementation defined message values
 * @param[out] msg               The response message
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_msg_send_direct_req_64(uint16_t source, uint16_t dest,
				      uint64_t a0, uint64_t a1, uint64_t a2,
				      uint64_t a3, uint64_t a4,
				      struct ffa_direct_msg *msg);

/**
 * @brief      Sends a 32 bit partition message in parameter registers as a
 *             response and blocks until the response is available.
 * @note       The ffa_interrupt_handler function can be called during the
 *             execution of this function
 *
 * @param[in]  source            Source endpoint ID
 * @param[in]  dest              Destination endpoint ID
 * @param[in]  a0,a1,a2,a3,a4    Implementation defined message values
 * @param[out] msg               The response message
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_msg_send_direct_resp_32(uint16_t source, uint16_t dest,
				       uint32_t a0, uint32_t a1, uint32_t a2,
				       uint32_t a3, uint32_t a4,
				       struct ffa_direct_msg *msg);

/**
 * @brief      Sends a 64 bit partition message in parameter registers as a
 *             response and blocks until the response is available.
 * @note       The ffa_interrupt_handler function can be called during the
 *             execution of this function
 *
 * @param[in]  source            Source endpoint ID
 * @param[in]  dest              Destination endpoint ID
 * @param[in]  a0,a1,a2,a3,a4    Implementation defined message values
 * @param[out] msg               The response message
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_msg_send_direct_resp_64(uint16_t source, uint16_t dest,
				       uint64_t a0, uint64_t a1, uint64_t a2,
				       uint64_t a3, uint64_t a4,
				       struct ffa_direct_msg *msg);

/**
 * Memory management interfaces
 *
 * @note Functions with _rxtx suffix use the RX/TX buffers mapped by
 * ffa_rxtx_map to transmit memory descriptors instead of an distinct buffer
 * allocated by the owner.
 */

/**
 * @brief      Starts a transaction to transfer of ownership of a memory region
 *             from a Sender endpoint to a Receiver endpoint.
 *
 * @param[in]  total_length     Total length of the memory transaction
 *                              descriptor in bytes
 * @param[in]  fragment_length  Length in bytes of the memory transaction
 *                              descriptor passed in this ABI invocation
 * @param[in]  buffer_address   Base address of a buffer allocated by the Owner
 *                              and distinct from the TX buffer
 * @param[in]  page_count       Number of 4K pages in the buffer allocated by
 *                              the Owner and distinct from the TX buffer
 * @param[out] handle           Globally unique Handle to identify the memory
 *                              region upon successful transmission of the
 *                              transaction descriptor.
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_mem_donate(uint32_t total_length, uint32_t fragment_length,
			  void *buffer_address, uint32_t page_count,
			  uint64_t *handle);

ffa_result ffa_mem_donate_rxtx(uint32_t total_length, uint32_t fragment_length,
			       uint64_t *handle);

/**
 * @brief      Starts a transaction to transfer an Owner’s access to a memory
 *             region and  grant access to it to one or more Borrowers.
 *
 * @param[in]  total_length     Total length of the memory transaction
 *                              descriptor in bytes
 * @param[in]  fragment_length  Length in bytes of the memory transaction
 *                              descriptor passed in this ABI invocation
 * @param[in]  buffer_address   Base address of a buffer allocated by the Owner
 *                              and distinct from the TX buffer
 * @param[in]  page_count       Number of 4K pages in the buffer allocated by
 *                              the Owner and distinct from the TX buffer
 * @param[out] handle           Globally unique Handle to identify the memory
 *                              region upon successful transmission of the
 *                              transaction descriptor.
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_mem_lend(uint32_t total_length, uint32_t fragment_length,
			void *buffer_address, uint32_t page_count,
			uint64_t *handle);

ffa_result ffa_mem_lend_rxtx(uint32_t total_length, uint32_t fragment_length,
			     uint64_t *handle);

/**
 * @brief      Starts a transaction to grant access to a memory region to one or
 *             more Borrowers.
 *
 * @param[in]  total_length     Total length of the memory transaction
 *                              descriptor in bytes
 * @param[in]  fragment_length  Length in bytes of the memory transaction
 *                              descriptor passed in this ABI invocation
 * @param[in]  buffer_address   Base address of a buffer allocated by the Owner
 *                              and distinct from the TX buffer
 * @param[in]  page_count       Number of 4K pages in the buffer allocated by
 *                              the Owner and distinct from the TX buffer
 * @param[out] handle           Globally unique Handle to identify the memory
 *                              region upon successful transmission of the
 *                              transaction descriptor.
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_mem_share(uint32_t total_length, uint32_t fragment_length,
			 void *buffer_address, uint32_t page_count,
			 uint64_t *handle);

ffa_result ffa_mem_share_rxtx(uint32_t total_length, uint32_t fragment_length,
			      uint64_t *handle);

/**
 * @brief      Requests completion of a donate, lend or share memory management
 *             transaction.
 *
 * @param[in]  total_length          Total length of the memory transaction
 *                                   descriptor in bytes
 * @param[in]  fragment_length       Length in bytes of the memory transaction
 *                                   descriptor passed in this ABI invocation
 * @param[in]  buffer_address        Base address of a buffer allocated by the
 *                                   Owner and distinct from the TX buffer
 * @param[in]  page_count            Number of 4K pages in the buffer allocated
 *                                   by the Owner and distinct from the TX
 *                                   buffer
 * @param[out] resp_total_length     Total length of the response memory
 *                                   transaction descriptor in bytes
 * @param[out] resp_fragment_length  Length in bytes of the response memory
 *                                   transaction descriptor passed in this ABI
 *                                   invocation
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_mem_retrieve_req(uint32_t total_length, uint32_t fragment_length,
				void *buffer_address, uint32_t page_count,
				uint32_t *resp_total_length,
				uint32_t *resp_fragment_length);

ffa_result ffa_mem_retrieve_req_rxtx(uint32_t total_length,
				     uint32_t fragment_length,
				     uint32_t *resp_total_length,
				     uint32_t *resp_fragment_length);

/**
 * @brief      Starts a transaction to transfer access to a shared or lent
 *             memory region from a Borrower back to its Owner.
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_mem_relinquish(void);

/**
 * @brief      Restores exclusive access to a memory region back to its Owner.
 *
 * @param[in]  handle  Globally unique Handle to identify the memory region
 * @param[in]  flags   Flags for modifying the reclaim behavior
 *
 * @return     The FF-A error status code
 */
ffa_result ffa_mem_reclaim(uint64_t handle, uint32_t flags);

/**
 * @brief       Queries the memory attributes of a memory region. This function
 *              can only access the regions of the SP's own translation regine.
 *              Moreover this interface is only available in the boot phase,
 *              i.e. before invoking FFA_MSG_WAIT interface.
 *
 * @param[in]   base_address    Base VA of a translation granule whose
 *                              permission attributes must be returned.
 * @param[out]  mem_perm        Permission attributes of the memory region
 *
 * @return      The FF-A error status code
 */
ffa_result ffa_mem_perm_get(const void *base_address, uint32_t *mem_perm);

/**
 * @brief       Sets the memory attributes of a memory regions. This function
 *              can only access the regions of the SP's own translation regine.
 *              Moreover this interface is only available in the boot phase,
 *              i.e. before invoking FFA_MSG_WAIT interface.
 *
 * @param[in]   base_address    Base VA of a memory region whose permission
 *                              attributes must be set.
 * @param[in]   page_count      Number of translation granule size pages
 *                              starting from the Base address whose permissions
 *                              must be set.
 * @param[in]   mem_perm        Permission attributes to be set for the memory
 *                              region
 * @return      The FF-A error status code
 */
ffa_result ffa_mem_perm_set(const void *base_address, uint32_t page_count,
			    uint32_t mem_perm);

/**
 * @brief 	Allow an entity to provide debug logging to the console. Uses
 * 		32 bit registers to pass characters.
 *
 * @param message	Message characters
 * @param length	Message length, max FFA_CONSOLE_LOG_32_MAX_LENGTH
 * @return 		The FF-A error status code
 */
ffa_result ffa_console_log_32(const char *message, size_t length);

/**
 * @brief 	Allow an entity to provide debug logging to the console. Uses
 * 		64 bit registers to pass characters.
 *
 * @param message	Message characters
 * @param length	Message length, max FFA_CONSOLE_LOG_64_MAX_LENGTH
 * @return 		The FF-A error status code
 */
ffa_result ffa_console_log_64(const char *message, size_t length);

/**
 * @brief      Interrupt handler prototype. Must be implemented by another
 *             component.
 *
 * @param[in]  interrupt_id  The interrupt identifier
 */
void ffa_interrupt_handler(uint32_t interrupt_id);

/**
 * @brief      VM created message handler prototype. Must be implemented by
 *             another component.
 *
 * @param[in]  vm_id  ID of VM that has been created
 * @param[in]  handle Globally unique Handle to identify a memory region that
 *                    contains information associated with the created VM
 * @return            The FF-A error status code
 */
ffa_result ffa_vm_created_handler(uint16_t vm_id, uint64_t handle);

/**
 * @brief      VM destroyed message handler prototype. Must be implemented by
 *             another component.
 *
 * @param[in]  vm_id  ID of VM that has been destroyed
 * @param[in]  handle Globally unique Handle to identify a memory region that
 *                    contains information associated with the destroyed VM
 * @return            The FF-A error status code
 */
ffa_result ffa_vm_destroyed_handler(uint16_t vm_id, uint64_t handle);

#ifdef __cplusplus
}
#endif

#endif /* LIBSP_INCLUDE_FFA_API_H_ */
