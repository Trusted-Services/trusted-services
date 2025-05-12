/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TS_PLATFORM_INTERFACE_DEVICE_REGION_H
#define TS_PLATFORM_INTERFACE_DEVICE_REGION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines a structure for describing a contiguous IO memory region
 * and other configuration information about a peripheral.  This may be based on
 * buildtime or runtime configuration information e.g. from device tree.
 */
struct device_region
{
    char dev_class[16];     /**< Identifier for class of device e.g. 'trng' */
    int dev_instance;       /**< Instance of the class of device on a platform */
    uintptr_t base_addr;    /**< Base address or region */
    uintptr_t phys_addr;    /**< Physical address of the region */
    size_t io_region_size;  /**< Size of I/O region in bytes */
};

#ifdef __cplusplus
}
#endif

#endif /* TS_PLATFORM_INTERFACE_DEVICE_REGION_H */
