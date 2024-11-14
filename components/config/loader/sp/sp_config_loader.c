// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 */

#include <common/fdt/fdt_helpers.h>
#include <config/interface/config_store.h>
#include <config/interface/config_blob.h>
#include <platform/interface/device_region.h>
#include <platform/interface/memory_region.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <trace.h>

#include "sp_config_loader.h"

/*
 * According to the FF-A spec: in the SP manifest the size of device and
 * memory regions is expressed as a count of 4K pages.
 */
#define FFA_SP_MANIFEST_PAGE_SIZE UINT32_C(0x1000)

struct sp_param_region {
	char name[16];
	uintptr_t location;
	size_t size;
};

static bool load_device_regions(const struct ffa_name_value_pair_v1_0 *value_pair)
{
	struct sp_param_region *d = (struct sp_param_region *)value_pair->value;

	/* Iterate over the device regions */
	while ((uintptr_t)d < (value_pair->value + value_pair->size)) {

		struct device_region device_region = { 0 };
		size_t name_length = strlen(d->name) + 1;

		if (name_length > sizeof(device_region.dev_class)) {
			EMSG("name too long");
			return false;
		}

		memcpy(device_region.dev_class, d->name, name_length);
		device_region.dev_instance = 0;
		device_region.base_addr = d->location;
		device_region.io_region_size = d->size;

		if (!config_store_add(CONFIG_CLASSIFIER_DEVICE_REGION,
				      device_region.dev_class,
				      device_region.dev_instance,
				      &device_region, sizeof(device_region))) {
			EMSG("failed to add device region to config store");
			return false;
		}

		++d;
	}

	return true;
}

static bool load_memory_regions(const struct ffa_name_value_pair_v1_0 *value_pair)
{
	struct sp_param_region *d = (struct sp_param_region *)value_pair->value;

	/* Iterate over the device regions */
	while ((uintptr_t)d < (value_pair->value + value_pair->size)) {

		struct memory_region memory_region = { 0 };
		size_t name_length = strlen(d->name) + 1;

		if (name_length > sizeof(memory_region.region_name)) {
			EMSG("name too long");
			return false;
		}

		memcpy(memory_region.region_name, d->name, name_length);
		memory_region.base_addr = d->location;
		memory_region.region_size = d->size;

		if (!config_store_add(CONFIG_CLASSIFIER_MEMORY_REGION,
				      memory_region.region_name, 0,
				      &memory_region, sizeof(memory_region))) {
			EMSG("failed to add memory region to config store");
			return false;
		}

		++d;
	}

	return true;
}

static bool load_blob(const struct ffa_name_value_pair_v1_0 *value_pair)
{
	struct config_blob blob;

	blob.data = (const void*)value_pair->value;
	blob.data_len = value_pair->size;

	if (!config_store_add(CONFIG_CLASSIFIER_BLOB, (const char *)value_pair->name, 0,
			      &blob, sizeof(blob))) {
		EMSG("failed to add blob to config store");
		return false;
	}

	return true;
}

static bool load_fdt(const void *fdt, size_t fdt_size)
{
	int root = -1, node = -1, subnode = -1, rc = -1;
	static const char *ffa_manifest_compatible = "arm,ffa-manifest-1.0";

	/* Sanity check */
	if (!fdt) {
		EMSG("fdt NULL pointer");
		return false;
	}

	rc = fdt_check_full(fdt, fdt_size);
	if (rc) {
		EMSG("fdt_check_full(): %d", rc);
		return false;
	}

	/* Find root node */
	root = fdt_path_offset(fdt, "/");
	if (root < 0) {
		EMSG("fdt_path_offset(): %d", root);
		return false;
	}

	/* Check if it's a valid SP manifest */
	rc = fdt_node_check_compatible(fdt, root, ffa_manifest_compatible);
	if (rc) {
		EMSG("fdt_node_check_compatible(%s): %d", ffa_manifest_compatible, rc);
		return false;
	}

	/* Find memory regions */
	node = fdt_node_offset_by_compatible(fdt, root, "arm,ffa-manifest-memory-regions");
	if (node >= 0) {
		fdt_for_each_subnode(subnode, fdt, node) {
			struct memory_region memory_region = {0};
			uint64_t base_addr = 0;
			uint32_t page_cnt = 0;
			const char *subnode_name = fdt_get_name(fdt, subnode, NULL);
			size_t name_length = 0;

			if (!subnode_name) {
				EMSG("subnode name is missing");
				return false;
			}

			if(!dt_get_u64(fdt, subnode, "base-address", &base_addr)) {
				EMSG("base-address is missing");
				return false;
			}

			if(!dt_get_u32(fdt, subnode, "pages-count", &page_cnt)) {
				EMSG("pages-count is missing");
				return false;
			}

			name_length = strlen(subnode_name) + 1;
			if (name_length > sizeof(memory_region.region_name)) {
				EMSG("name too long");
				return false;
			}

			memcpy(memory_region.region_name, subnode_name, name_length);
			memory_region.base_addr = (uintptr_t)base_addr;
			memory_region.region_size = page_cnt * FFA_SP_MANIFEST_PAGE_SIZE;

			if (!config_store_add(CONFIG_CLASSIFIER_MEMORY_REGION,
					      memory_region.region_name, 0,
					      &memory_region, sizeof(memory_region))) {
				EMSG("failed to add memory region to config store");
				return false;
			}
		}
	}

	/* Find device regions */
	node = fdt_node_offset_by_compatible(fdt, root, "arm,ffa-manifest-device-regions");
	if (node >= 0) {
		fdt_for_each_subnode(subnode, fdt, node) {
			struct device_region device_region = {0};
			uint64_t base_addr = 0;
			uint32_t page_cnt = 0;
			const char *subnode_name = fdt_get_name(fdt, subnode, NULL);
			size_t name_length = 0;

			if (!subnode_name) {
				EMSG("subnode name is missing");
				return false;
			}

			if(!dt_get_u64(fdt, subnode, "base-address", &base_addr)) {
				EMSG("base-address is missing");
				return false;
			}

			if (!dt_get_u32(fdt, subnode, "pages-count", &page_cnt)) {
				EMSG("pages-count is missing");
				return false;
			}

			name_length = strlen(subnode_name) + 1;
			if (name_length > sizeof(device_region.dev_class)) {
				EMSG("name too long");
				return false;
			}

			memcpy(device_region.dev_class, subnode_name, name_length);
			device_region.base_addr = base_addr;
			device_region.io_region_size = page_cnt * FFA_SP_MANIFEST_PAGE_SIZE;
			device_region.dev_instance = 0;

			if (!config_store_add(CONFIG_CLASSIFIER_DEVICE_REGION,
					      device_region.dev_class, device_region.dev_instance,
					      &device_region, sizeof(device_region))) {
				EMSG("failed to add device region to config store");
				return false;
			}
		}
	}

	/* Find TPM event log */
	node = fdt_node_offset_by_compatible(fdt, root, "arm,tpm_event_log");
	if (node >= 0) {
		uint64_t tpm_event_log_addr = 0;
		uint32_t tpm_event_log_size = 0;
		struct config_blob blob = { 0 };

		if (!dt_get_u64(fdt, node, "tpm_event_log_addr", &tpm_event_log_addr)) {
			EMSG("tpm_event_log_addr is missing");
			return false;
		}

		if (!dt_get_u32(fdt, node, "tpm_event_log_size", &tpm_event_log_size)) {
			EMSG("tpm_event_log_size is missing");
			return false;
		}

		blob.data = (const void *)tpm_event_log_addr;
		blob.data_len = tpm_event_log_size;

		if (!config_store_add(CONFIG_CLASSIFIER_BLOB, "EVENT_LOG", 0,
				      (void *)&blob, sizeof(blob))) {
			EMSG("failed to add event log to config store");
			return false;
		}
	}

	/* Find hardware features */
	node = fdt_node_offset_by_compatible(fdt, root, "arm,hw-features");
	if (node >= 0) {
		const char *prop_name = NULL;
		uint32_t prop_value = 0;
		int prop_offset = 0;

		fdt_for_each_property_offset(prop_offset, fdt, node) {
			if (!dt_get_u32_by_offset(fdt, prop_offset, &prop_name, &prop_value)) {
				/* skip other properties in the node, e.g. the compatible string */
				DMSG("skipping non-u32 property '%s' in hw-features", prop_name);
				continue;
			}

			if (!config_store_add(CONFIG_CLASSIFIER_HW_FEATURE, prop_name, 0,
					      &prop_value, sizeof(prop_value))) {
				EMSG("failed to add HW feature to config store");
				return false;
			}
		}
	} else {
		DMSG("arm,hw-features node not present in SP manifest");
	}

	return true;
}

static bool sp_config_load_v1_0(struct ffa_boot_info_v1_0 *boot_info)
{
	/* Load deployment specific configuration */
	for (size_t param_index = 0; param_index < boot_info->count; param_index++) {
		const char *name = (const char *)boot_info->nvp[param_index].name;
		const size_t name_max_size = sizeof(boot_info->nvp[param_index].name);

		if (!strncmp(name, "DEVICE_REGIONS", name_max_size)) {
			if (!load_device_regions(&boot_info->nvp[param_index])) {
				EMSG("Failed to load device regions");
				return false;
			}
		} else if (!strncmp(name, "MEMORY_REGIONS", name_max_size)) {
			if (!load_memory_regions(&boot_info->nvp[param_index])) {
				EMSG("Failed to load memory regions");
				return false;
			}
		} else if (!memcmp(name, "TYPE_DT\0\0\0\0\0\0\0\0", name_max_size)) {
			if (!load_fdt((void *)boot_info->nvp[param_index].value,
					boot_info->nvp[param_index].size)) {
				EMSG("Failed to load SP config from DT");
				return false;
			}
		} else {
			if (!load_blob(&boot_info->nvp[param_index])) {
				EMSG("Failed to load blob");
				return false;
			}
		}
	}

	return true;
}

static bool sp_config_load_v1_1(struct ffa_boot_info_header_v1_1 *boot_info_header)
{
	size_t desc_end = 0;
	size_t total_desc_size = 0;
	struct ffa_boot_info_desc_v1_1 *boot_info_desc = NULL;
	uint32_t expected_version = SHIFT_U32(1, FFA_VERSION_MAJOR_SHIFT) |
				    SHIFT_U32(1, FFA_VERSION_MINOR_SHIFT);

	if (boot_info_header->version != expected_version) {
		EMSG("Invalid FF-A boot info version");
		return false;
	}

	if (boot_info_header->desc_size != sizeof(struct ffa_boot_info_desc_v1_1)) {
		EMSG("Boot info descriptor size mismatch");
		return false;
	}

	if (MUL_OVERFLOW(boot_info_header->desc_size, boot_info_header->desc_cnt,
			 &total_desc_size)) {
		EMSG("Boot info descriptor overflow");
		return false;
	}

	if (ADD_OVERFLOW(boot_info_header->desc_offs, total_desc_size, &desc_end) ||
	    boot_info_header->size < desc_end) {
		EMSG("Boot info descriptor overflow");
		return false;
	}

	boot_info_desc = (struct ffa_boot_info_desc_v1_1 *)((uintptr_t)boot_info_header +
							    boot_info_header->desc_offs);

	for (unsigned int i = 0; i < boot_info_header->desc_cnt; i++) {
		uint16_t flags = FFA_BOOT_INFO_CONTENT_FMT_ADDR << FFA_BOOT_INFO_CONTENT_FMT_SHIFT;
		uint16_t type = FFA_BOOT_INFO_TYPE_STD << FFA_BOOT_INFO_TYPE_SHIFT |
				FFA_BOOT_INFO_ID_STD_FDT << FFA_BOOT_INFO_ID_SHIFT;

		if (boot_info_desc[i].flags == flags && boot_info_desc[i].type == type) {
			if (!load_fdt((void *)boot_info_desc->contents, boot_info_desc->size)) {
				EMSG("Failed to load SP config FDT");
				return false;
			}
		}
	}

	return true;
}

bool sp_config_load(union ffa_boot_info *boot_info)
{
	if (!boot_info)
		return false;

	switch (boot_info->signature) {
	case FFA_BOOT_INFO_SIGNATURE_V1_0:
		return sp_config_load_v1_0((struct ffa_boot_info_v1_0 *)&boot_info->boot_info_v1_0);
	case FFA_BOOT_INFO_SIGNATURE_V1_1:
		return sp_config_load_v1_1((struct ffa_boot_info_header_v1_1 *)
					   &boot_info->boot_info_v1_1);
	default:
		EMSG("Invalid FF-A boot info signature");
		return false;
	}

	return false;
}
