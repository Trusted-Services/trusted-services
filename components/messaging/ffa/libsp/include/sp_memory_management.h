/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_INCLUDE_SP_MEMORY_MANAGEMENT_H_
#define LIBSP_INCLUDE_SP_MEMORY_MANAGEMENT_H_

/**
 * @file  sp_memory_management.h
 * @brief The file contains wrapper functions around the FF-A interfaces
 *        described in section 8 of the specification.
 */

#include "ffa_memory_descriptors.h"
#include "sp_api_defines.h"
#include "sp_api_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
/**
 * @brief      Security state of the memory region
 */
enum sp_memory_security_state {
	sp_memory_security_state_secure = 0x00,
	sp_memory_security_state_non_secure = 0x01,
};
#endif /* CFG_FFA_VERSION */

/**
 * @brief      Memory type of the region
 */
enum sp_memory_type {
	sp_memory_type_not_specified = 0x00,	/**< Not specified */
	sp_memory_type_device_memory = 0x01,	/**< Device memory */
	sp_memory_type_normal_memory = 0x02,	/**< Normal memory */
	sp_memory_type_reserved = 0x03		/**< Reserved */
};

/**
 * @brief      Cacheability attribute of the memory region
 */
enum sp_cacheability_attribute {
	sp_cacheability_reserved0 = 0x00,	/**< Reserved */
	sp_cacheability_non_cacheable = 0x01,	/**< Non-cacheable */
	sp_cacheability_reserved2 = 0x02,	/**< Reserved */
	sp_cacheability_write_back = 0x03	/**< Write-back */
};

/**
 * @brief      Device memory attributes (Gathering, Reordering, Early Write
 *             Acknowledgment) of the memory region
 */
enum sp_device_memory_attributes {
	sp_device_memory_nGnRnE = 0x00,	/**< nGnRnE */
	sp_device_memory_nGnRE = 0x01,	/**< nGnRE */
	sp_device_memory_nGRE = 0x02,	/**< nGRE */
	sp_device_memory_GRE = 0x03	/**< GRE */
};

/**
 * @brief      Shareability attribute of the memory region
 */
enum sp_shareablity_attribute {
	sp_shareability_non_shareable = 0x00,	/**< Non-shareable */
	sp_shareability_reserved = 0x01,	/**< Reserved */
	sp_shareability_outer_shareable = 0x02,	/**< Outer shareable */
	sp_shareability_inner_shareable = 0x03	/**< Inner shareable */
};

/**
 * @brief      Instruction access permission of the memory region
 */
enum sp_instruction_access_permission {
	sp_instruction_access_not_specified = 0x00,	/**< Not specified*/
	sp_instruction_access_not_executable = 0x01,	/**< Not executable */
	sp_instruction_access_executable = 0x02,	/**< Executable */
	sp_instruction_access_reserved = 0x03		/**< Reserved */
};

/**
 * @brief      Data access permission of the memory region
 */
enum sp_data_access_permission {
	sp_data_access_not_specified = 0x00,	/**< Not specified */
	sp_data_access_read_only = 0x01,	/**< Read-only */
	sp_data_access_read_write = 0x02,	/**< Read-write */
	sp_data_access_reserved = 0x03		/**< Reserved */
};

/**
 * @brief      Memory management transaction type
 */
enum sp_memory_transaction_type {
	/** Relayer specified */
	sp_memory_transaction_type_relayer_specified = 0x00,
	sp_memory_transaction_type_share = 0x01,	/**< Share */
	sp_memory_transaction_type_lend = 0x02,		/**< Lend */
	sp_memory_transaction_type_donate = 0x03,	/**< Donate */
};

/**
 * @brief       This union contains the memory attributes for normal and device
 *              memory areas.
 */
union sp_memory_attr {
	/** Normal memory attributes */
	struct sp_normal_memory_attributes {
		/** Cacheability attribute */
		enum sp_cacheability_attribute cacheability;

		/** Shareability attribute */
		enum sp_shareablity_attribute shareability;
	} normal_memory;

	/** Device memory attributes */
	enum sp_device_memory_attributes device_memory;

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
	/* Memory security state (secure/non-secure) */
	enum sp_memory_security_state security_state;
#endif /* CFG_FFA_VERSION */
};

/**
 * @brief      Flags are used to govern the behavior of a memory management
 *             transaction.
 */
struct sp_memory_transaction_flags {
	/**
	 * Zero memory on share, lend and donate call or zero memory before
	 * retrieval flag on a retrieve call or zero memory after relinquish on
	 * a relinquish call.
	 */
	bool zero_memory;

	/**
	 * The flag specifies if the relayer can time slice this operation.
	 */
	bool operation_time_slicing;

	/**
	 * The flag specified if the relayer must zero the memory region
	 * contents after unmapping it from the borrower's translation regime or
	 * if the borrower crashes.
	 * **This field can only be used at a retrieve call.**
	 */
	bool zero_memory_after_relinquish;

	/**
	 * In an invocation of a retrieve call, this flag is used by the
	 * receiver to either specify the memory management transaction it is
	 * participating in or indicate that it will discover this information
	 * in the response structure.
	 * **This field can only be used at a retrieve call.**
	 */
	enum sp_memory_transaction_type transaction_type;

	/**
	 * This flag is used by the receiver to specify the boundary.
	 * The valid range is 8K-120K in 8K steps. Zero means the relayer must
	 * choose the alignment boundary. The value of this field is multiplied
	 * by 8K.
	 *  **This field can only be used at a retrieve call.**
	 */
	uint32_t alignment_hint;
};

/**
 * @brief      This structure describes the details of a memory transaction. The
 *             type is used in donate, lend, share and retrieve calls but the
 *             mandatory and optional fields vary according to the description
 *             of the individual fields.
 */
struct sp_memory_descriptor {
	uint16_t sender_id;	/**< Sender FF-A ID, **must** be specified */

	/**
	 * The type of the memory region (device, normal, not specified)
	 * * Donate: The owner **must not** specify it
	 * * Lend: The owner **must not** specify it
	 * * Share: The owner **must** specify it
	 * * Retrieve: The receiver **should** specify same or less permissive
	 *             as the owner. This field is filled with the value used by
	 *             the relayer on retrieve.
	 */
	enum sp_memory_type memory_type;

	/**
	 * Memory region attributes
	 * The field of the union which is matching with the memory type should
	 * be used. In case of non-specified memory type the contents of the
	 * union is ignored.
	 */
	union sp_memory_attr mem_region_attr;

	/**
	 * Flags are used to govern the behavior of a memory management
	 * transaction.
	 * The allowed flags for each transaction type is describe in the type's
	 * comment.
	 */
	struct sp_memory_transaction_flags flags;

	/**
	 *  This 64-bit field must be used to specify an implementation defined
	 *  value associated with the transaction and known to participating
	 *  endpoints.
	 */
	uint64_t tag;
};

/**
 * @brief      Structure for specifying access restrictions for
 *             receiver endpoints.
 */
struct sp_memory_access_descriptor {
	uint16_t receiver_id;	/**< Receiver FF-A ID, **must** be specified */

	/**
	 * Instruction access permission of the memory region
	 * * Donate: The owner **must not** specify it
	 * * Lend: The owner **must not** specify it
	 * * Share: The owner **must not** specify it
	 * * Retrieve: The receiver **must not** specify it on retrieving a
	 *             **shared or lent** memory region. The receiver **should**
	 *             specify it on retrieving a **donated** region. This field
	 *             is filled with the value used by the relayer on retrieve.
	 */
	enum sp_instruction_access_permission instruction_access;

	/**
	 * Data access permission of the memory region
	 * * Donate: The owner **must not** specify it
	 * * Lend: The owner **must** specify it
	 * * Share: The owner **must** specify it
	 * * Retrieve: The receiver **must** specify it on retrieving a
	 *             **donated** memory region. The receiver **should**
	 *             specify same or less permissive as the owner on
	 *             retrieving **shared or lent** memory region.This field
	 *             is filled with the value used by the relayer on retrieve.
	 */
	enum sp_data_access_permission data_access;
};

/**
 * @brief      The structure describes a memory region by its base address
 *             and its page count.
 */
struct sp_memory_region {
	void *address;		/**< Base address of the memory region */
	uint32_t page_count;	/**< Number of 4K pages in memory region */
};

/**
 * @brief	Enum for describing data access permissions in
 *		sp_memory_permission_get/set calls (FFA_MEM_PERM_GET/SET).
 */
enum sp_mem_perm_data_access_permission {
	sp_mem_perm_data_perm_no_access = 0x00,
	sp_mem_perm_data_perm_read_write = 0x01,
	sp_mem_perm_data_perm_reserved = 0x02,
	sp_mem_perm_data_perm_read_only = 0x03
};

/**
 * @brief	Enum for describing instruction access permissions in
 *		sp_memory_permission_get/set calls (FFA_MEM_PERM_GET/SET).
 */
enum sp_mem_perm_instruction_access_permission {
	sp_mem_perm_instruction_perm_executable = 0x00,
	sp_mem_perm_instruction_perm_non_executable = 0x01
};

/**
 * @brief	Struct for describing memory access setting in
 *		sp_memory_permission_get/set calls (FFA_MEM_PERM_GET/SET).
 *
 */
struct sp_mem_perm {
	enum sp_mem_perm_data_access_permission data_access;
	enum sp_mem_perm_instruction_access_permission instruction_access;
};

/**
 * @brief      Starts a transaction to transfer of ownership of a memory region
 *             from a Sender endpoint to a Receiver endpoint.
 *
 * @param[in]  descriptor    The memory descriptor
 * @param[in]  acc_desc      Access descriptor
 * @param[in]  regions       Memory region array
 * @param[in]  region_count  Memory region count
 * @param[out] handle        The handle for identifying the memory transaction
 *
 * @return     The SP API result
 */
sp_result sp_memory_donate(struct sp_memory_descriptor *descriptor,
			   struct sp_memory_access_descriptor *acc_desc,
			   struct sp_memory_region regions[],
			   uint32_t region_count, uint64_t *handle);

sp_result sp_memory_donate_dynamic(struct sp_memory_descriptor *descriptor,
				   struct sp_memory_access_descriptor *acc_desc,
				   struct sp_memory_region regions[],
				   uint32_t region_count, uint64_t *handle,
				   struct ffa_mem_transaction_buffer *buffer);

/**
 * @brief      Queries if sp_memory_donate_dynamic is supported by the SPM.
 *
 * @param[out] supported  Flag indicating the support state
 *
 * @return     The SP API result
 */
sp_result sp_memory_donate_dynamic_is_supported(bool *supported);

/**
 * @brief      Starts a transaction to transfer an Owner’s access to a memory
 *             region and  grant access to it to one or more Borrowers.
 *
 * @param[in]  descriptor      The memory descriptor
 * @param[in]  acc_desc        Access descriptor array
 * @param[in]  acc_desc_count  Access descriptor count, must be greater than 0
 * @param[in]  regions         Memory region array
 * @param[in]  region_count    Memory region count
 * @param[out] handle          The handle for identifying the memory transaction
 *
 * @return     The SP API result
 */
sp_result sp_memory_lend(struct sp_memory_descriptor *descriptor,
			 struct sp_memory_access_descriptor acc_desc[],
			 uint32_t acc_desc_count,
			 struct sp_memory_region regions[],
			 uint32_t region_count, uint64_t *handle);

sp_result sp_memory_lend_dynamic(struct sp_memory_descriptor *descriptor,
				 struct sp_memory_access_descriptor acc_desc[],
				 uint32_t acc_desc_count,
				 struct sp_memory_region regions[],
				 uint32_t region_count, uint64_t *handle,
				 struct ffa_mem_transaction_buffer *buffer);

/**
 * @brief      Queries if sp_memory_lend_dynamic is supported by the SPM.
 *
 * @param[out] supported  Flag indicating the support state
 *
 * @return     The SP API result
 */
sp_result sp_memory_lend_dynamic_is_supported(bool *supported);

/**
 * @brief      Starts a transaction to grant access to a memory region to one or
 *             more Borrowers.
 *
 * @param[in]  descriptor      The memory descriptor
 * @param[in]  acc_desc        Access descriptor array
 * @param[in]  acc_desc_count  Access descriptor count, must be greater than 0
 * @param[in]  regions         Memory region array
 * @param[in]  region_count    Memory region count
 * @param[out] handle          The handle for identifying the memory transaction
 *
 * @return     The SP API result
 */
sp_result sp_memory_share(struct sp_memory_descriptor *descriptor,
			  struct sp_memory_access_descriptor acc_desc[],
			  uint32_t acc_desc_count,
			  struct sp_memory_region regions[],
			  uint32_t region_count, uint64_t *handle);

sp_result sp_memory_share_dynamic(struct sp_memory_descriptor *descriptor,
				  struct sp_memory_access_descriptor acc_desc[],
				  uint32_t acc_desc_count,
				  struct sp_memory_region regions[],
				  uint32_t region_count, uint64_t *handle,
				  struct ffa_mem_transaction_buffer *buffer);

/**
 * @brief      Queries if sp_memory_share_dynamic is supported by the SPM.
 *
 * @param[out] supported  Flag indicating the support state
 *
 * @return     The SP API result
 */
sp_result sp_memory_share_dynamic_is_supported(bool *supported);

/**
 * @brief          Requests completion of a donate, lend or share memory
 *                 management transaction.
 *
 * @param[in]      descriptor        The memory descriptor
 * @param[in,out]  acc_desc          Access descriptor
 * @param[in,out]  regions           Memory region array
 * @param[in]      in_region_count   Count of the specified regions, can be 0
 * @param[in,out]  out_region_count  Count of the reserved space of in the
 *                                   regions buffer for retrieved regions. After
 *                                   call it will contain the exact count of the
 *                                   retrieved regions.
 * @param[in]      handle            The handle for identifying the memory
 *                                   transaction
 *
 * @return     The SP API result
 */
sp_result sp_memory_retrieve(struct sp_memory_descriptor *descriptor,
			     struct sp_memory_access_descriptor *acc_desc,
			     struct sp_memory_region regions[],
			     uint32_t in_region_count,
			     uint32_t *out_region_count, uint64_t handle);

sp_result
sp_memory_retrieve_dynamic(struct sp_memory_descriptor *descriptor,
			   struct sp_memory_access_descriptor *acc_desc,
			   struct sp_memory_region regions[],
			   uint32_t in_region_count, uint32_t *out_region_count,
			   uint64_t handle,
			   struct ffa_mem_transaction_buffer *buffer);

/**
 * @brief      Queries if sp_memory_retrieve_dynamic is supported by the SPM.
 *
 * @param[out] supported  Flag indicating the support state
 *
 * @return     The SP API result
 */
sp_result sp_memory_retrieve_dynamic_is_supported(bool *supported);

/**
 * @brief      Starts a transaction to transfer access to a shared or lent
 *             memory region from a Borrower back to its Owner.
 *
 * @param[in]  handle          The handle for identifying the memory transaction
 * @param[in]  endpoints       The endpoints
 * @param[in]  endpoint_count  The endpoint count
 *
 * @return     The SP API result
 */
sp_result sp_memory_relinquish(uint64_t handle, const uint16_t endpoints[],
			       uint32_t endpoint_count,
			       struct sp_memory_transaction_flags *flags);

/**
 * @brief      Restores exclusive access to a memory region back to its Owner.
 *
 * @param[in]  handle  The handle for identifying the memory transaction
 * @param[in]  flags   Flags for modifying the reclaim behavior
 *
 * @return     The SP API result
 */
sp_result sp_memory_reclaim(uint64_t handle, uint32_t flags);

/**
 * @brief       Queries the memory attributes of a memory region. It can only
 *              access the regions of the SP's own translation regine. Moreover
 *              this function is only available in the boot phase, i.e. before
 *              calling sp_msg_wait.
 *
 *
 * @param[in]   base_address    Base VA of a translation granule whose
 *                              permission attributes must be returned.
 * @param[out]  mem_perm        Permission attributes of the memory region
 * @return      The SP API result
 */
sp_result sp_memory_permission_get(const void *base_address,
				   struct sp_mem_perm *mem_perm);

/**
 * @brief       Sets the memory attributes of a memory regions. It can only
 *              access the regions of the SP's own translation regine. Moreover
 *              this function is only available in the boot phase, i.e. before
 *              calling sp_msg_wait.
 *
 * @param[in]   base_address    Base VA of a memory region whose permission
 *                              attributes must be set.
 * @param[in]   region_size     Memory regions size in bytes
 * @param[in]   mem_perm        Permission attributes to be set for the memory
 *                              region
 * @return      The SP API result
 */
sp_result sp_memory_permission_set(const void *base_address, size_t region_size,
				   const struct sp_mem_perm *mem_perm);

#ifdef __cplusplus
}
#endif

#endif /* LIBSP_INCLUDE_SP_MEMORY_MANAGEMENT_H_ */
