/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <cstring>
#include <limits>
#include <protocols/rpc/common/packed-c/encoding.h>
#include <service/uefi/smm_variable/client/cpp/smm_variable_client.h>
#include <service_locator.h>

/*
 * Attack tests for the smm-variable service.  Invalid parameters are sent by
 * a client to test service provider defenses against a potential attack.
 */
TEST_GROUP(SmmVariableAttackTests)
{
	void setup()
	{
		m_rpc_session = NULL;
		m_service_context = NULL;

		service_locator_init();

		m_service_context = service_locator_query("sn:trustedfirmware.org:smm-variable:0");
		CHECK_TRUE(m_service_context);

		m_rpc_session = service_context_open(m_service_context);
		CHECK_TRUE(m_rpc_session);

		m_client = new smm_variable_client(m_rpc_session);

		setup_common_guid();
		var_name_1=to_variable_name(u"varibale_1");
		null_name=to_variable_name(u"");
		var_name_test=to_variable_name(u"test_variable");

	}

	void teardown()
	{
		delete m_client;
		m_client = NULL;

		if (m_service_context) {
			if (m_rpc_session) {
				service_context_close(m_service_context, m_rpc_session);
				m_rpc_session = NULL;
			}

			service_context_relinquish(m_service_context);
			m_service_context = NULL;
		}
	}

	void setup_common_guid()
	{
		m_common_guid.Data1 = 0x12341234;
		m_common_guid.Data2 = 0x1234;
		m_common_guid.Data3 = 0x1234;
		m_common_guid.Data4[0] = 0x00;
		m_common_guid.Data4[1] = 0x01;
		m_common_guid.Data4[2] = 0x02;
		m_common_guid.Data4[3] = 0x03;
		m_common_guid.Data4[4] = 0x04;
		m_common_guid.Data4[5] = 0x05;
		m_common_guid.Data4[6] = 0x06;
		m_common_guid.Data4[7] = 0x07;
	}

	std::u16string to_variable_name(const char16_t *string)
	{
		std::u16string var_name(string);
		var_name.push_back(0);

		return var_name;
	}

	smm_variable_client *m_client;
	struct rpc_caller_session *m_rpc_session;
	struct service_context *m_service_context;
	EFI_GUID m_common_guid;
	std::u16string var_name_1;
	std::u16string null_name;
	std::u16string var_name_test;
};

TEST(SmmVariableAttackTests, setWithOversizeData)
{
	efi_status_t efi_status = EFI_SUCCESS;
	std::string set_data = "UEFI variable data string";

	/* Override the data size with a big but plausable length */
	efi_status = m_client->set_variable(m_common_guid, var_name_test, set_data, 0, 0,
					    std::numeric_limits<uint16_t>::max());

	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableAttackTests, setWithSizeMaxDataSize)
{
	efi_status_t efi_status = EFI_SUCCESS;
	std::string set_data = "UEFI variable data string";

	/* Override the data size with SIZE_MAX length */
	efi_status = m_client->set_variable(m_common_guid, var_name_test, set_data, 0, 0,
					    std::numeric_limits<size_t>::max());

	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableAttackTests, setWithOversizeName)
{
	efi_status_t efi_status = EFI_SUCCESS;
	std::string set_data = "UEFI variable data string";

	/* Override the name size slightly too big such that name + data don't fit */
	efi_status = m_client->set_variable(m_common_guid, var_name_test, set_data, 0,
					    (var_name_test.size() + 1) * sizeof(uint16_t) + 1, 0);

	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableAttackTests, setWithSizeMaxNameSize)
{
	efi_status_t efi_status = EFI_SUCCESS;
	std::string set_data = "UEFI variable data string";

	/* Override the name size slightly too big such that name + data don't fit */
	efi_status = m_client->set_variable(m_common_guid, var_name_test, set_data, 0,
					    std::numeric_limits<size_t>::max(), 0);

	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableAttackTests, setAndGetWithOversizeName)
{
	efi_status_t efi_status = EFI_SUCCESS;
	std::string set_data = "UEFI variable data string";
	std::string get_data;

	efi_status = m_client->set_variable(m_common_guid, var_name_test, set_data, 0);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name_test, get_data,
					    (var_name_test.size() + 1) * sizeof(int16_t) + 1);

	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);

	/* Expect remove to be permitted */
	efi_status = m_client->remove_variable(m_common_guid, var_name_test);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
}

TEST(SmmVariableAttackTests, setAndGetWithSizeMaxNameSize)
{
	efi_status_t efi_status = EFI_SUCCESS;
	const char16_t var_name[] = u"test_variable";
	std::string set_data = "UEFI variable data string";
	std::string get_data;

	efi_status = m_client->set_variable(m_common_guid, var_name, set_data, 0);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data,
					    std::numeric_limits<size_t>::max());

	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);

	/* Expect remove to be permitted */
	efi_status = m_client->remove_variable(m_common_guid, var_name);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
}

TEST(SmmVariableAttackTests, enumerateWithSizeMaxNameSize)
{
	efi_status_t efi_status = EFI_SUCCESS;
	std::u16string var_name=null_name;
	EFI_GUID guid;
	memset(&guid, 0, sizeof(guid));

	/* Add a variable */
	efi_status = m_client->set_variable(m_common_guid, var_name_1, std::string("Some data"),
					    EFI_VARIABLE_NON_VOLATILE);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Initial iteration uses good name length for next variable */
	efi_status = m_client->get_next_variable_name(guid, var_name, std::numeric_limits<size_t>::max());

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Next iteration uses invalid name length, so a null terminator can not fit */
	var_name = null_name;
	efi_status = m_client->get_next_variable_name(guid, var_name, 1);

	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);

	/* Next iteration uses invalid name length, so a null terminator can not fit */
	var_name = null_name;
	efi_status = m_client->get_next_variable_name(guid, var_name, 2);

	UNSIGNED_LONGLONGS_EQUAL(EFI_BUFFER_TOO_SMALL, efi_status);

	/* Expect to be able to remove the variable */
	efi_status = m_client->remove_variable(m_common_guid, var_name_1);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
}

TEST(SmmVariableAttackTests, setCheckPropertyWithOversizeName)
{
	efi_status_t efi_status = EFI_SUCCESS;

	VAR_CHECK_VARIABLE_PROPERTY check_property;
	check_property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
	check_property.Attributes = 0;
	check_property.Property = 0;
	check_property.MinSize = 0;
	check_property.MaxSize = 200;

	efi_status = m_client->set_var_check_property(m_common_guid, var_name_1, check_property,
						      (var_name_1.size() + 1) * sizeof(int16_t) + 1);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableAttackTests, setCheckPropertyWithMaxSizeName)
{
	efi_status_t efi_status = EFI_SUCCESS;

	VAR_CHECK_VARIABLE_PROPERTY check_property;
	check_property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
	check_property.Attributes = 0;
	check_property.Property = 0;
	check_property.MinSize = 0;
	check_property.MaxSize = 200;

	efi_status = m_client->set_var_check_property(m_common_guid, var_name_1, check_property,
						      std::numeric_limits<size_t>::max());
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableAttackTests, getCheckPropertyWithOversizeName)
{
	efi_status_t efi_status = EFI_SUCCESS;
	VAR_CHECK_VARIABLE_PROPERTY check_property;

	efi_status = m_client->get_var_check_property(m_common_guid, var_name_1, check_property,
						      (var_name_1.size() + 1) * sizeof(int16_t) + 1);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableAttackTests, getCheckPropertyWithMaxSizeName)
{
	efi_status_t efi_status = EFI_SUCCESS;

	VAR_CHECK_VARIABLE_PROPERTY check_property;

	efi_status = m_client->get_var_check_property(m_common_guid, var_name_1, check_property,
						      std::numeric_limits<size_t>::max());
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}
