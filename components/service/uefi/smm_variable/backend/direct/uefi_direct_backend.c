/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <mbedtls/build_info.h>
#include <mbedtls/pkcs7.h>
#include <mbedtls/x509_crt.h>
#include "common/mbedtls/mbedtls_utils.h"
#include <protocols/common/efi/efi_status.h>
#include <stdint.h>
#include <string.h>
#include <compiler.h>

int verify_pkcs7_signature(const uint8_t *signature_cert, uint64_t signature_cert_len,
			   const uint8_t *hash, uint64_t hash_len, const uint8_t *public_key_cert,
			   uint64_t public_key_cert_len)
{
	int mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;

	/* Parse the PKCS#7 DER encoded signature block */
	mbedtls_pkcs7 pkcs7_structure;

	mbedtls_pkcs7_init(&pkcs7_structure);

	mbedtls_status = mbedtls_pkcs7_parse_der(&pkcs7_structure, signature_cert,
							signature_cert_len);

	if (mbedtls_status == MBEDTLS_PKCS7_SIGNED_DATA) {

		/*
		 * If a separate public key is provided, verify the signature with it,
		 * else use the key from the pkcs7 signature structure, because it is
		 * a self-signed certificate.
		 */
		if(public_key_cert_len) {
			/* Parse the public key certificate */
			mbedtls_x509_crt signer_certificate;

			mbedtls_x509_crt_init(&signer_certificate);

			mbedtls_status = mbedtls_x509_crt_parse_der(&signer_certificate, public_key_cert,
								public_key_cert_len);

			if (mbedtls_status == 0) {
				/* Verify hash against signed hash */
				mbedtls_status = mbedtls_pkcs7_signed_hash_verify(
					&pkcs7_structure, &signer_certificate, hash, hash_len);
			}

			mbedtls_x509_crt_free(&signer_certificate);
		} else {
			mbedtls_status = mbedtls_pkcs7_signed_hash_verify(
				&pkcs7_structure, &pkcs7_structure.private_signed_data.private_certs, hash, hash_len);
		}
	}

	mbedtls_pkcs7_free(&pkcs7_structure);

	return mbedtls_status;
}

int get_uefi_priv_auth_var_fingerprint_handler(const uint8_t *signature_cert,
				    uint64_t signature_cert_len,
				    uint8_t *output)
{
	int mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;

	/* Parse the PKCS#7 DER encoded signature block */
	mbedtls_pkcs7 pkcs7_structure;

	mbedtls_pkcs7_init(&pkcs7_structure);

	mbedtls_status = mbedtls_pkcs7_parse_der(&pkcs7_structure, signature_cert,
							signature_cert_len);

	if (mbedtls_status == MBEDTLS_PKCS7_SIGNED_DATA) {

		//uint8_t output_buffer[PSA_HASH_MAX_SIZE] =  { 0 };
		size_t __maybe_unused output_size = 0;
		const mbedtls_asn1_buf *signerCertCN = NULL;
		const mbedtls_x509_crt *topLevelCert = &pkcs7_structure.private_signed_data.private_certs;
		const mbedtls_x509_buf *toplevelCertTbs = NULL;
		psa_hash_operation_t op = PSA_HASH_OPERATION_INIT;

		/* Find common name field of the signing certificate, which is the first in the chain */
		signerCertCN = findCommonName(&topLevelCert->subject);
		if (!signerCertCN) {
			mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
			goto end;
		}

		/* Get the TopLevel certificate which is the last in the chain */
		while(topLevelCert->next)
			topLevelCert = topLevelCert->next;
		toplevelCertTbs = &topLevelCert->tbs;

		/* Hash the data to create the fingerprint */
		op = psa_hash_operation_init();

		if (psa_hash_setup(&op, PSA_ALG_SHA_256) != PSA_SUCCESS) {
			mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
			goto end;
		}

		if (psa_hash_update(&op, signerCertCN->p, signerCertCN->len)) {
			psa_hash_abort(&op);
			mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
			goto end;
		}

		if (psa_hash_update(&op, toplevelCertTbs->p, toplevelCertTbs->len)) {
			psa_hash_abort(&op);
			mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
			goto end;
		}

		if (psa_hash_finish(&op, output, PSA_HASH_MAX_SIZE, &output_size)) {
			psa_hash_abort(&op);
			mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
			goto end;
		}

		/* Clear the remaining part of the buffer for consistency */
		memset(output + output_size, 0, PSA_HASH_MAX_SIZE - output_size);
	}

end:
	mbedtls_pkcs7_free(&pkcs7_structure);

	return (mbedtls_status == MBEDTLS_PKCS7_SIGNED_DATA) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;
}
