/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <config/interface/config_store.h>
#include <platform/interface/trng.h>
#include <platform/interface/device_region.h>
#include <psa/error.h>
#include <tztrng.h>
#include <tztrng_defs.h>
#include <stdlib.h>
#include <limits.h>

/*
 * A platform trng driver that uses the tz-trng driver to provide a
 * hardware entropy source.
 */
struct tztrng_instance
{
    struct device_region trng_device_region;
};


static int trng_poll(void *context, unsigned char *output, size_t nbyte, size_t *len)
{
    struct tztrng_instance *this_instance = (struct tztrng_instance*)context;
    int status = 0;

    *len = 0;

    if (nbyte >= sizeof(unsigned char)) {

        if (this_instance) {

            status = CC_TrngGetSource((unsigned long)this_instance->trng_device_region.base_addr,
                            output, len, nbyte * CHAR_BIT);
        }
        else {
            /* No context for TRNG instance */
            /*   status = LLF_RND_STATE_PTR_INVALID_ERROR;  @todo mbedcrypto segfaults when an error is returned */
            *len = sizeof(unsigned char);
        }
    }

    return status;
}

int platform_trng_create(struct platform_trng_driver *driver, int instance)
{
    static const struct platform_trng_iface iface =  { .poll = trng_poll };
    struct device_region device_region;
    struct tztrng_instance *new_instance = NULL;

    /*
     * Default to leaving the driver in a safe but inoperable state.
     */
    driver->iface = &iface;
    driver->context = NULL;

    if (!config_store_query(CONFIG_CLASSIFIER_DEVICE_REGION, "trng",
                            instance, &device_region,
                            sizeof(device_region)))
        return PSA_STATUS_HARDWARE_FAILURE;

    /*
     * A device region has been provided, possibly from an external configuration.
     * Check that it's a sensible size to defend against a bogus configuration.
     */
    new_instance = malloc(sizeof(struct tztrng_instance));

    if (!new_instance)
        return PSA_ERROR_INSUFFICIENT_MEMORY;

    new_instance->trng_device_region = device_region;
    driver->context = new_instance;

    return PSA_SUCCESS;
}

void platform_trng_destroy(struct platform_trng_driver *driver)
{
    free(driver->context);
}
