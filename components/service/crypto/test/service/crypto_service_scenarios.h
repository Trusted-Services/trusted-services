/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CRYPTO_SERVICE_SCENARIOS_H
#define CRYPTO_SERVICE_SCENARIOS_H

#include <service/crypto/client/cpp/crypto_client.h>

/*
 * Service-level test scenarios for the crypto service that may be reused using
 * different concrete crypto_clients to check end-to-end operation using different
 * protocol serialization schemes.
 */
class crypto_service_scenarios
{
public:
	crypto_service_scenarios(crypto_client *crypto_client);
	~crypto_service_scenarios();

	void generateRandomNumbers();
	void asymEncryptDecrypt();
	void asymEncryptDecryptWithSalt();
	void signAndVerifyHash();
	void signAndVerifyMessage();
	void signAndVerifyEat();
	void exportAndImportKeyPair();
	void exportPublicKey();
	void generatePersistentKeys();
	void generateVolatileKeys();
	void copyKey();
	void purgeKey();
	void verifypkcs7signature(void);
	void getUefiPrivAuthVarFingerprint(void);

private:
	crypto_client *m_crypto_client;
};

#endif /* CRYPTO_SERVICE_SCENARIOS_H */
