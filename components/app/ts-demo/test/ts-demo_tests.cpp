/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <app/ts-demo/ts-demo.h>
#include <protocols/rpc/common/packed-c/encoding.h>
#include <service/crypto/client/cpp/crypto_client.h>
#include <service/crypto/client/cpp/protocol/packed-c/packedc_crypto_client.h>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include <service_locator.h>

TEST_GROUP(TsDemoTests)
{
	void setup()
	{
		m_rpc_session = NULL;
		m_crypto_service_context = NULL;
		m_crypto_client = NULL;

		service_locator_init();

		m_crypto_service_context = service_locator_query("sn:trustedfirmware.org:crypto:0");
		CHECK(m_crypto_service_context);

		m_rpc_session = service_context_open(m_crypto_service_context);
		CHECK(m_rpc_session);

		(void)psa_crypto_client_init(m_rpc_session);

		m_crypto_client = new packedc_crypto_client(m_rpc_session);
	}

	void teardown()
	{
		delete m_crypto_client;
		m_crypto_client = NULL;

		if (m_crypto_service_context) {
			if (m_rpc_session) {
				service_context_close(m_crypto_service_context, m_rpc_session);
				m_rpc_session = NULL;
			}

			psa_crypto_client_deinit();
			service_context_relinquish(m_crypto_service_context);
			m_crypto_service_context = NULL;
		}
	}

	struct rpc_caller_session *m_rpc_session;
	struct service_context *m_crypto_service_context;
	crypto_client *m_crypto_client;
};

TEST(TsDemoTests, runTsDemo)
{
	int status = run_ts_demo(false);
	CHECK_EQUAL(0, status);
}
