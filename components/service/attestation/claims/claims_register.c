/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include "claims_register.h"

/* Maximum registered claim_sources for a deployment */
#define CLAIMS_REGISTER_MAX_CLAIM_SOURCES           (20)

/**
 * The singleton claims_register instance.
 */
static struct claims_register
{
    size_t num_sources;
    struct claim_source *sources[CLAIMS_REGISTER_MAX_CLAIM_SOURCES];

} instance;

static void query_collection_by_category(struct claim *collection,
                    enum claim_category category, struct claim_vector *result);



void claims_register_init(void)
{
    memset(&instance, 0, sizeof(instance));
}

void claims_register_deinit(void)
{
    instance.num_sources = 0;
}

void claims_register_add_claim_source(uint32_t category_map,
                                struct claim_source *source)
{
    if (source && (instance.num_sources < CLAIMS_REGISTER_MAX_CLAIM_SOURCES)) {

        source->category_map = category_map;

        instance.sources[instance.num_sources] = source;
        ++instance.num_sources;
    }
}

void claims_register_query_by_category(enum claim_category category,
                            struct claim_vector *result)
{
    /* Iterate over all claim_sources and gather qualifying claims.
     * A claim_source may source different categories of claim.  e.g.
     * an event log is a flexible logging mechanism that can hold
     * arbitrary claims.
     */
    for (size_t i = 0; i < instance.num_sources; ++i) {

        struct claim_source *source = instance.sources[i];

        if (source->category_map & category) {

            struct claim claim;

            if (claim_source_get_claim(source, &claim)) {

                if (claim.variant_id == CLAIM_VARIANT_ID_COLLECTION) {

                    query_collection_by_category(&claim, category, result);
                }
                else if (claim.category == category) {

                    claim_vector_push_back(result, &claim);
                }
            }
        }
    }
}

static void query_collection_by_category(struct claim *collection,
                    enum claim_category category, struct claim_vector *result)
{
    struct claim_iterator iter;

    claim_collection_variant_create_iterator(&collection->variant.collection, &iter);

    while (!iter.is_done(&iter)) {

        struct claim claim;

        iter.current(&iter, &claim);

        if (claim.category == category) {

            claim_vector_push_back(result, &claim);
        }

        iter.next(&iter);
    }
}
