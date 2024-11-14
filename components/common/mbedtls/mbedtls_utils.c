/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include "mbedtls_utils.h"

/*
 * Official value: http://www.oid-info.com/get/2.5.4.3
 * Hex converter: https://misc.daniel-marschall.de/asn.1/oid-converter/online.php
 */
#define CN_OID_TAG (0x06)
#define CN_OID_LEN (0x03)
#define CN_OID_VAL {0x55, 0x04, 0x03}

/* Searches for the common name field in an mbedtls_asn1_named_data object */
const mbedtls_asn1_buf* findCommonName(const mbedtls_asn1_named_data *name)
{
	static const uint8_t cn_oid_values[CN_OID_LEN] = CN_OID_VAL;

	while (name)
	{
		if (name->oid.tag == CN_OID_TAG && name->oid.len == CN_OID_LEN) {
			if (name->oid.p != NULL) {
				if (!memcmp(name->oid.p, cn_oid_values, (size_t) CN_OID_LEN))
					return &name->val;
			}
		}

		name = name->next;
	}

	return NULL;
}
