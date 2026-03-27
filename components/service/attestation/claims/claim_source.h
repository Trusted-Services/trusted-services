/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLAIM_SOURCE_H
#define CLAIM_SOURCE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct claim;

/**
 * An abstract interface for getting a claim from some source.  The source
 * may return any type of claim variant so it could be a single claim or a
 * collection.
 */
struct claim_source
{
    bool (*get_claim)(void *context, struct claim *claim);
    void *context;

    /**
     * A bitmap of claim categories that this claim_source provides claims for.
     * Claim categories are enumerated by enum claim_category.
     */
    uint32_t category_map;

};

/**
 * \brief Get a claim from the claim source
 *
 * \param[in] cs         The claim source
 * \param[out] claim     The returned claim
 *
 * \return Returns true if a claim is returned.
 */
static inline bool claim_source_get_claim(struct claim_source *cs, struct claim *claim)
{
    return cs->get_claim(cs->context, claim);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CLAIM_SOURCE_H */
