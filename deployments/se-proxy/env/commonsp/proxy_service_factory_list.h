/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PROXY_SERVICE_FACTORY_LIST_H_
#define _PROXY_SERVICE_FACTORY_LIST_H_

#include "components/rpc/common/endpoint/rpc_service_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/** file: proxy_service_factory_list.h
 * Linker constructed array for service proxy factory
 *
 * This header defines C types, variables and macros allowing to build an array of service factory
 * functions enabled in the deployment. Any factory function marked by ADD_PROXY_SERVICE_FACTORY
 * will be added to the array if the .c file holding the mark is included in the build. This allows
 * build time configuring of the deployment.
 *
 * Functions in the array shall be called by order of placement, which can be controlled by an
 * integer value. Lower number means earlier placement and call order.
 *
 * The file relies on the "linker constructed array" design pattern. This requires array members to
 * be assigned to a specific section, and proper entries in the linker command file to place the
 * members adjoining to each other in memory, to stop the linker garbage collecting the items, and
 * to define a symbol to the start of the array. For ld (the GNU linker) the following works:
 *
 *      // Construct an array from factory initializers
 *      . = ALIGN(8);
 *      __proxy_service_factory_list = .;
 *      KEEP(*(SORT_BY_NAME(.proxy_service_factory_list.*)));
 *      __proxy_service_factory_list_end = .;
 *
 *  Because the structures contain a function pointer, the generated section
 *  will be writable if the compilation is position-independent. Therefore, the
 *  linker command file should place the array in an appropriate segment.
 */

/** Interface priority for service endpoints. Lower number gives lower interface ID. */
#define SE_PROXY_INTERFACE_PRIO_ITS		0
#define SE_PROXY_INTERFACE_PRIO_PS		1
#define SE_PROXY_INTERFACE_PRIO_CRYPTO		2
/* The below prio must be after SE_PROXY_INTERFACE_PRIO_CRYPTO */
#define SE_PROXY_INTERFACE_PRIO_CRYPTO_NANO	3
#define SE_PROXY_INTERFACE_PRIO_ATTEST		4
#define SE_PROXY_INTERFACE_PRIO_FWU		5

/** A structure to allow assigning a name to the factory pointer for logging purposes. */
struct se_proxy_list_entry {
	struct rpc_service_interface *(* const fn)(void);
	const char name[32];
};

#define _STR_HELPER(x) #x
#define _STR(x) _STR_HELPER(x)

/** A macro to add a new array member.
 *
 * @fn:       Factory function to call.
 * @name:     Name to use in trace messages.
 * @ifc_prio: Priority. Determines the order of array items. Two items with the same priority will
 *            be placed after each other in undetermined order. Smaller numbers mean higher priority
 *            and will result in lower index in the array.
 *
 */
#define ADD_PROXY_SERVICE_FACTORY(fn, name, ifc_prio) \
	static const struct se_proxy_list_entry list_entry_##name  __attribute__((used, \
			section(".proxy_service_factory_list." _STR(ifc_prio)))) =  {(fn), (#name)}

/** A symbol referring to the start of the array. */
extern const struct se_proxy_list_entry __proxy_service_factory_list[];

/** A symbol referring to last item +1 of the array. */
extern const struct se_proxy_list_entry __proxy_service_factory_list_end[];

/** Evaluating to the length (number of item) of the service factory list. */
#define PROXY_SERVICE_FACTORY_LIST_LENGTH() (((uintptr_t)__proxy_service_factory_list_end - \
				  (uintptr_t)__proxy_service_factory_list) \
				  / sizeof(struct se_proxy_list_entry))

#ifdef __cplusplus
}
#endif

#endif
