/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <codecvt>
#include <cstring>
#include <locale>
#include <sstream>
#include <limits>

#include "util.h"

#if defined(UEFI_AUTH_VAR)
#include "auth_vectors/KEK.h"
#include "auth_vectors/KEK_delete.h"
#include "auth_vectors/PK1.h"
#include "auth_vectors/PK1_delete.h"
#include "auth_vectors/PK2.h"
#include "auth_vectors/PK2_delete.h"
#include "auth_vectors/PK3.h"
#include "auth_vectors/db1.h"
#include "auth_vectors/db2.h"
#include "auth_vectors/var.h"
#include "auth_vectors/var_append.h"
#include "auth_vectors/var_append_old.h"
#include "auth_vectors/var_data.h"
#include "auth_vectors/var_delete.h"
#include "auth_vectors/var_delete_old.h"
#endif
#include "common/uuid/uuid.h"
#include "protocols/rpc/common/packed-c/encoding.h"
#include "service/uefi/smm_variable/client/cpp/smm_variable_client.h"
#include "service_locator.h"

/*
 * Service-level tests for the smm-variable service.
 */
TEST_GROUP(SmmVariableServiceTests)
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
	}

	void teardown()
	{
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, cleanupPersistentStore());

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

	std::u16string to_variable_name(const char16_t *string)
	{
		std::u16string var_name(string);
		var_name.push_back(0);

		return var_name;
	}

	std::string guid_to_string(EFI_GUID *guid)
	{
		struct uuid_canonical canonical_uuid;

		/* EFI_GUID is similar to the UUID octet structure */
		uuid_canonical_from_guid_octets(&canonical_uuid, (const struct uuid_octets *)guid);

		return std::string(canonical_uuid.characters);
	}

	/*
	 * Adds a UEFI variable in a formatted way to a buffer. e.g:
	 *     name: Bob
	 *     guid: {0x01234567, 0x89ab, 0xCDEF, {0x1, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef }}
	 */
	void addVariableToMessage(std::u16string &var_name, EFI_GUID *guid, std::string &out)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

		/* Remove null terminator! */
		out += "\tname: ";
		out += (convert.to_bytes(var_name)).substr(0, var_name.size() - 1);
		out += "\n\tguid: ";
		out += guid_to_string(guid);
		out += "\n";
	}

	bool isVariableUncleanable(std::u16string &var_name)
	{
		for(std::u16string* element : m_non_rm_vars) {
			if (!var_name.compare(*element))
				return true;
		}

		return false;
	}

	/* Clear all the removable variables from the persistent store. */
	efi_status_t cleanupPersistentStore()
	{
		std::string error_message;
		std::u16string var_name = to_variable_name(u"");
		std::string get_data;
		EFI_GUID guid;
		efi_status_t status;

		memset(&guid, 0, sizeof(guid));

#if defined(UEFI_AUTH_VAR)

		/* Remove PK if exists */
		status = m_client->get_variable(m_global_guid, u"PK", get_data);

		if (status == EFI_SUCCESS) {
			/*
			* PK must be cleared to disable authentication so other
			* variables can be deleted easily. Return value is not
			* checked, because if there is no PK in the store it
			* will not be found.
			*/
			status = m_client->set_variable(m_global_guid, u"PK", PK1_delete_auth,
							sizeof(PK1_delete_auth),
							m_authentication_common_attributes);

			if (status)
				FAIL("Cannot remove PK at cleanup!");
		}

		/*
		 * Note:
		 * If a test fills PK with data not removable by PK1_delete_auth request
		 * the proper request has to be added here or at the end of the specific test!
		 */
#endif

		do {
			status = m_client->get_next_variable_name(guid, var_name, max_variable_size);

			/* There are no more variables in the persistent store */
			if (status == EFI_NOT_FOUND) {
				status = EFI_SUCCESS;
				break;
			} else if (status != EFI_SUCCESS) {
				addVariableToMessage(var_name, &guid, error_message);
				break;
			}

			/* Skip variables that cannot be deleted */
			if (isVariableUncleanable(var_name))
				continue;

			status = m_client->remove_variable(guid, var_name);

#if defined(UEFI_AUTH_VAR)
			/*
			 * Authenticated variables can be removed only with authentication
			 * header. If the variable could not be removed with a normal call,
			 * try again with an empty header.
			 */
			if (status) {
				status = m_client->set_variable(guid, var_name, m_empty_auth_header,
								sizeof(m_empty_auth_header),
								m_authentication_common_attributes);
			}
#endif
			if (status) {
				addVariableToMessage(var_name, &guid, error_message);
				break;
			}

			/*
			 * If the variable was successfully removed the fields are cleared so
			 * the iteration will start again from the first available variable.
			 * If the remove is unsuccessful (for example the variable is not
			 * accessible from runtime or read only) the fields are kept so the
			 * iteration searches for the next one. This case there could be a
			 * combination when this function tries to remove non-removable
			 * variables multiple times.
			 * */
			if (status == EFI_SUCCESS) {
				var_name = to_variable_name(u"");
				memset(&guid, 0, sizeof(guid));
			}

		} while (1);

		if (error_message.length() != 0) {
			error_message.insert(0, "Could not remove variable: \n");
			UT_PRINT(error_message.c_str());
		}

		return status;
	}

	smm_variable_client *m_client;
	struct rpc_caller_session *m_rpc_session;
	struct service_context *m_service_context;

	/*
	 * List of the variables which cannot be deleted.
	 * E.g read-only ones or those that have boot only access
	 */
	std::u16string m_ro_variable = to_variable_name(u"ro_variable");
	std::u16string m_boot_finished_var_name = to_variable_name(u"finished");

	uint32_t max_variable_size = 4096;

	/* Cleanup skips these variables */
	std::vector<std::u16string *> m_non_rm_vars{ &m_ro_variable, &m_boot_finished_var_name };

	EFI_GUID m_common_guid = { 0x01234567,
				   0x89ab,
				   0xCDEF,
				   { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef } };

#if defined(UEFI_AUTH_VAR)
	EFI_GUID m_global_guid = { 0x8BE4DF61,
				   0x93CA,
				   0x11d2,
				   { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };
	EFI_GUID m_security_database_guid = { 0xd719b2cb,
					      0x3d3a,
					      0x4596,
					      { 0xa3, 0xbc, 0xda, 0xd0, 0xe, 0x67, 0x65, 0x6f } };
	uint32_t m_authentication_common_attributes =
		EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
		EFI_VARIABLE_BOOTSERVICE_ACCESS |
		EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
	uint8_t m_empty_auth_header[40] = { /* TimeStamp in the far future */
					    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					    /* dwLength = header size - sizeof (TimeStamp) */
					    24, 0, 0, 0,
					    /* wRevision = WIN_CERT_CURRENT_VERSION */
					    0x00, 0x02,
					    /* wCertificateType = WIN_CERT_TYPE_EFI_GUID */
					    0xF1, 0x0E,
					    /* CertType = EFI_CERT_TYPE_PKCS7_GUID */
					    0x9d, 0xd2, 0xaf, 0x4a, 0xdf, 0x68, 0xee, 0x49, 0x8a,
					    0xa9, 0x34, 0x7d, 0x37, 0x56, 0x65, 0xa7
	};
#endif
};

TEST(SmmVariableServiceTests, setAndGet)
{
	efi_status_t efi_status = EFI_SUCCESS;
	const char16_t var_name[] = u"test_variable";
	std::string set_data = "UEFI variable data string";
	std::string get_data;

	efi_status = m_client->set_variable(m_common_guid, var_name, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	UNSIGNED_LONGS_EQUAL(set_data.size(), get_data.size());
	STRCMP_EQUAL(set_data.c_str(), get_data.c_str());

	/* Extend the variable using an append write */
	std::string append_data = " values added with append write";

	efi_status = m_client->set_variable(m_common_guid, var_name, append_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS |
						    EFI_VARIABLE_APPEND_WRITE);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	std::string appended_data = set_data + append_data;

	/* Expect the append write operation to have extended the variable */
	UNSIGNED_LONGLONGS_EQUAL(appended_data.size(), get_data.size());
	STRCMP_EQUAL(appended_data.c_str(), get_data.c_str());

	/* Expect remove to be permitted */
	efi_status = m_client->remove_variable(m_common_guid, var_name);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);
}

TEST(SmmVariableServiceTests, setAndGetNv)
{
	efi_status_t efi_status = EFI_SUCCESS;
	const char16_t var_name[] = u"an NV test_variable";
	std::string set_data = "Another UEFI variable data string";
	std::string get_data;

	efi_status = m_client->set_variable(m_common_guid, var_name, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS |
						    EFI_VARIABLE_NON_VOLATILE);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	UNSIGNED_LONGS_EQUAL(set_data.size(), get_data.size());
	STRCMP_EQUAL(set_data.c_str(), get_data.c_str());

	/* Extend the variable using an append write */
	std::string append_data = " values added with append write";

	efi_status = m_client->set_variable(
		m_common_guid, var_name, append_data,
		EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS |
			EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_APPEND_WRITE);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	std::string appended_data = set_data + append_data;

	/* Expect the append write operation to have extended the variable */
	UNSIGNED_LONGLONGS_EQUAL(appended_data.size(), get_data.size());
	STRCMP_EQUAL(appended_data.c_str(), get_data.c_str());

	/* Expect remove to be permitted */
	efi_status = m_client->remove_variable(m_common_guid, var_name);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);
}

TEST(SmmVariableServiceTests, getVarSize)
{
	efi_status_t efi_status = EFI_SUCCESS;
	const char16_t var_name[] = u"test_variable";
	std::string set_data = "UEFI variable data string";
	std::string get_data;

	efi_status = m_client->set_variable(m_common_guid, var_name, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Get with the data size set to zero. This is the standard way
	 * to discover the variable size. */
	efi_status = m_client->get_variable(m_common_guid, var_name, get_data, 0, 0);

	UNSIGNED_LONGLONGS_EQUAL(EFI_BUFFER_TOO_SMALL, efi_status);
	UNSIGNED_LONGS_EQUAL(set_data.size(), get_data.size());

	/* Expect remove to be permitted */
	efi_status = m_client->remove_variable(m_common_guid, var_name);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);
}

TEST(SmmVariableServiceTests, getVarSizeNv)
{
	efi_status_t efi_status = EFI_SUCCESS;
	const char16_t var_name[] = u"test_variable";
	std::string set_data = "UEFI variable data string";
	std::string get_data;

	efi_status = m_client->set_variable(m_common_guid, var_name, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS |
						    EFI_VARIABLE_NON_VOLATILE);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Get with the data size set to zero. This is the standard way
	 * to discover the variable size. */
	efi_status = m_client->get_variable(m_common_guid, var_name, get_data, 0, 0);

	UNSIGNED_LONGLONGS_EQUAL(EFI_BUFFER_TOO_SMALL, efi_status);
	UNSIGNED_LONGS_EQUAL(set_data.size(), get_data.size());

	/* Expect remove to be permitted */
	efi_status = m_client->remove_variable(m_common_guid, var_name);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->get_variable(m_common_guid, var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);
}

/* This test makes the irreversible transition from boot to runtime
 * state, leaving a variable that can't be removed. To prevent this from
 * breaking the variable enumeration test, this test is called from
 * the enumeration test to guarantee the order.
 */
TEST(SmmVariableServiceTests, runtimeStateAccessControl)
{
	efi_status_t efi_status = EFI_SUCCESS;

	std::string boot_finished_var_data = "Set after bootstate is finished";
	const char16_t boot_var_name[] = u"a boot variable";
	std::string boot_set_data = "Only accessible during boot";
	const char16_t runtime_var_name[] = u"a runtime variable";
	std::string runtime_set_data = "Only accessible during runtime";
	std::string get_data;

	efi_status = m_client->get_variable(m_common_guid, m_boot_finished_var_name, get_data);
	if (efi_status == EFI_SUCCESS) {
		UT_PRINT("runtimeStateAccessControl testcase can only run once per boot cycle. "
			 "It exits boot state, blocking access to the added boot variables, "
			 "so the test is skipped for now.");
		return;
	}

	/* The variable should be successfully found or NOT_FOUND. Other values are related to errors */
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);

	/* Add variables with runtime state access control */
	efi_status = m_client->set_variable(m_common_guid, boot_var_name, boot_set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status =
		m_client->set_variable(m_common_guid, runtime_var_name, runtime_set_data,
				       EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
					       EFI_VARIABLE_BOOTSERVICE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Expect access to boot variable to be permitted */
	efi_status = m_client->get_variable(m_common_guid, boot_var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
	UNSIGNED_LONGS_EQUAL(boot_set_data.size(), get_data.size());
	STRCMP_EQUAL(boot_set_data.c_str(), get_data.c_str());

	/* Expect access to the runtime variable to also be permitted during boot */
	efi_status = m_client->get_variable(m_common_guid, runtime_var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Exit boot service - access should no longer be permitted */
	efi_status = m_client->exit_boot_service();
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Access to the boot variable should now be forbidden */
	efi_status = m_client->get_variable(m_common_guid, boot_var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);

	/* Expect access to the runtime variable should still be permitted */
	efi_status = m_client->get_variable(m_common_guid, runtime_var_name, get_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
	UNSIGNED_LONGS_EQUAL(runtime_set_data.size(), get_data.size());
	STRCMP_EQUAL(runtime_set_data.c_str(), get_data.c_str());

	/* Expect removing boot variable to be forbidden */
	efi_status = m_client->remove_variable(m_common_guid, boot_var_name);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);

	/* Expect removing runtime variable to be permitted */
	efi_status = m_client->remove_variable(m_common_guid, runtime_var_name);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Set a non-volatile read-only variable marking the end of the boot phase */
	efi_status = m_client->set_variable(
		m_common_guid, m_boot_finished_var_name, boot_finished_var_data,
		EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
			EFI_VARIABLE_RUNTIME_ACCESS);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Apply a check to constrain to Read Only */
	VAR_CHECK_VARIABLE_PROPERTY check_property;
	check_property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
	check_property.Attributes = 0;
	check_property.Property = VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
	check_property.MinSize = 0;
	check_property.MaxSize = 100;

	efi_status = m_client->set_var_check_property(m_common_guid, m_boot_finished_var_name,
						      check_property);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
}

/* This test leaves a read-only variable in the store */
TEST(SmmVariableServiceTests, readOnlyConstraint)
{
	efi_status_t efi_status = EFI_SUCCESS;

	std::string set_data = "A read only variable";
	std::string get_data;

	/* Only add the read only variable if it is not already created */
	efi_status = m_client->get_variable(m_common_guid, m_ro_variable, get_data);

	if (efi_status != EFI_SUCCESS) {
		/* Add a variable to the store */
		efi_status = m_client->set_variable(m_common_guid, m_ro_variable, set_data,
						    EFI_VARIABLE_NON_VOLATILE |
							    EFI_VARIABLE_BOOTSERVICE_ACCESS |
							    EFI_VARIABLE_RUNTIME_ACCESS);

		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

		/* Apply a check to constrain to Read Only */
		VAR_CHECK_VARIABLE_PROPERTY check_property;
		check_property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
		check_property.Attributes = 0;
		check_property.Property = VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
		check_property.MinSize = 0;
		check_property.MaxSize = 100;

		efi_status = m_client->set_var_check_property(m_common_guid, m_ro_variable,
							      check_property);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

		/* Read back the check property constraint and expect it to match the set value */
		VAR_CHECK_VARIABLE_PROPERTY got_check_property;

		efi_status = m_client->get_var_check_property(m_common_guid, m_ro_variable,
							      got_check_property);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

		UNSIGNED_LONGS_EQUAL(check_property.Revision, got_check_property.Revision);
		UNSIGNED_LONGS_EQUAL(check_property.Attributes, got_check_property.Attributes);
		UNSIGNED_LONGS_EQUAL(check_property.Property, got_check_property.Property);
		UNSIGNED_LONGS_EQUAL(check_property.MinSize, got_check_property.MinSize);
		UNSIGNED_LONGS_EQUAL(check_property.MaxSize, got_check_property.MaxSize);
	} else {
		UT_PRINT("Read-only variable already exists");
	}

	/* Attempt to modify variable */
	efi_status = m_client->set_variable(
		m_common_guid, m_ro_variable, std::string("Different variable data"),
		EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS);

	UNSIGNED_LONGLONGS_EQUAL(EFI_WRITE_PROTECTED, efi_status);

	/* Expect to still be able to read variable */
	efi_status = m_client->get_variable(m_common_guid, m_ro_variable, get_data);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Variable value should be unmodified */
	UNSIGNED_LONGS_EQUAL(set_data.size(), get_data.size());
	STRCMP_EQUAL(set_data.c_str(), get_data.c_str());
}

TEST(SmmVariableServiceTests, enumerateStoreContents)
{
	efi_status_t efi_status = EFI_SUCCESS;
	std::string get_data;

	/* Query information about the variable store */
	size_t nv_max_variable_storage_size = 0;
	size_t nv_max_variable_size = 0;
	size_t nv_remaining_variable_storage_size = 0;

	efi_status = m_client->query_variable_info(EFI_VARIABLE_NON_VOLATILE,
						   &nv_max_variable_storage_size,
						   &nv_remaining_variable_storage_size,
						   &nv_max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	size_t v_max_variable_storage_size = 0;
	size_t v_max_variable_size = 0;
	size_t v_remaining_variable_storage_size = 0;

	efi_status = m_client->query_variable_info(0, &v_max_variable_storage_size,
						   &v_remaining_variable_storage_size,
						   &v_max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Add some variables to the store */
	std::u16string var_name_1 = to_variable_name(u"variable_1");
	std::u16string var_name_2 = to_variable_name(u"variable_2");
	std::u16string var_name_3 = to_variable_name(u"variable_3");
	std::string set_data = "Some variable data";

	efi_status = m_client->set_variable(m_common_guid, var_name_1, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS |
						    EFI_VARIABLE_NON_VOLATILE);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->set_variable(m_common_guid, var_name_2, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS |
						    EFI_VARIABLE_NON_VOLATILE);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->set_variable(m_common_guid, var_name_3, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Query variable info again and check it's as expected */
	size_t max_variable_storage_size = 0;
	size_t max_variable_size = 0;
	size_t remaining_variable_storage_size = 0;

	/* Check non-volatile - two variables have been added */
	efi_status =
		m_client->query_variable_info(EFI_VARIABLE_NON_VOLATILE, &max_variable_storage_size,
					      &remaining_variable_storage_size, &max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
	UNSIGNED_LONGLONGS_EQUAL((nv_remaining_variable_storage_size - set_data.size() * 2),
				 remaining_variable_storage_size);

	/* Check volatile - one variable has been added */
	efi_status = m_client->query_variable_info(0, &max_variable_storage_size,
						   &remaining_variable_storage_size,
						   &max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
	UNSIGNED_LONGLONGS_EQUAL((v_remaining_variable_storage_size - set_data.size() * 1),
				 remaining_variable_storage_size);

	/*
	 * Enumerate store contents. We know nothing about the original state of the store
	 * (there can be read only variables from previous test runs) so our new variables
	 * are added to a list and we check if all of them are added by iterating through
	 * all elements of the store.
	 */
	std::u16string var_name = to_variable_name(u"");
	EFI_GUID guid;
	memset(&guid, 0, sizeof(guid));

	std::u16string *expected_variables[] = { &var_name_1, &var_name_2, &var_name_3 };

	do {
		efi_status = m_client->get_next_variable_name(guid, var_name, max_variable_size);
		if (efi_status != EFI_SUCCESS)
			break;

		for (unsigned int i = 0; i < ARRAY_SIZE(expected_variables); i++) {
			/* Skipp NULL elements*/
			if (!expected_variables[i])
				continue;
			/* Check if the found variable is in the expected list */
			if (!var_name.compare(*expected_variables[i])) {
				if ((*expected_variables[i]).size() == var_name.size())
					expected_variables[i] = NULL;
			}
		}
	} while (1);

	// Fail if sore content enumeration failed.
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, efi_status);

	for (unsigned int i = 0; i < ARRAY_SIZE(expected_variables); i++) {
		if (expected_variables[i])
			FAIL("Not all variables were found in the store");
	}

	/* Expect to be able to remove all variables */
	efi_status = m_client->remove_variable(m_common_guid, var_name_1);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->remove_variable(m_common_guid, var_name_2);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	efi_status = m_client->remove_variable(m_common_guid, var_name_3);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
}

TEST(SmmVariableServiceTests, setSizeConstraint)
{
	efi_status_t efi_status = EFI_SUCCESS;
	const char16_t var_name_1[] = u"size_limited_variable";
	std::string set_data = "Initial value";

	/* Add a variable to the store */
	efi_status = m_client->set_variable(m_common_guid, var_name_1, set_data,
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Apply a check to constrain the variable size */
	VAR_CHECK_VARIABLE_PROPERTY check_property;
	check_property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
	check_property.Attributes = 0;
	check_property.Property = 0;
	check_property.MinSize = 0;
	check_property.MaxSize = 20;

	efi_status = m_client->set_var_check_property(m_common_guid, var_name_1, check_property);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Attempt to set value to a size that exceeds the MaxSize constraint */
	efi_status = m_client->set_variable(
		m_common_guid, var_name_1,
		std::string("A data value that exceeds the MaxSize constraint"),
		EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);

	/* But setting a value that's within the constraints should work */
	efi_status = m_client->set_variable(m_common_guid, var_name_1, std::string("Small value"),
					    EFI_VARIABLE_BOOTSERVICE_ACCESS |
						    EFI_VARIABLE_RUNTIME_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Removing should be allowed though */
	efi_status = m_client->remove_variable(m_common_guid, var_name_1);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	/* Although the variable has been removed, the constraint should
	 * still be set.
	 */
	efi_status = m_client->set_variable(
		m_common_guid, var_name_1,
		std::string("Another try to set a value that exceeds the MaxSize constraint"),
		EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, efi_status);
}

TEST(SmmVariableServiceTests, checkMaxVariablePayload)
{
	efi_status_t efi_status = EFI_SUCCESS;
	size_t max_payload_size = 0;

	/* Expect to read a reasonable size for the variable payload */
	efi_status = m_client->get_payload_size(max_payload_size);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);
	CHECK_TRUE(max_payload_size >= 1024);
	CHECK_TRUE(max_payload_size <= 64 * 1024);
}

#if defined(UEFI_AUTH_VAR)
TEST(SmmVariableServiceTests, privAuthVarSetAndGet)
{
	efi_status_t status;
	std::string read_data;

	/* Set variable */
	status = m_client->set_variable(m_common_guid, u"var", VAR_auth, sizeof(VAR_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Overwrite variable with the same timestamp */
	status = m_client->set_variable(m_common_guid, u"var", VAR_auth, sizeof(VAR_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Append variable with the same timestamp */
	status = m_client->set_variable(m_common_guid, u"var", VAR_append_auth, sizeof(VAR_append_auth),
					m_authentication_common_attributes | EFI_VARIABLE_APPEND_WRITE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Get and validate variable value */
	status = m_client->get_variable(m_common_guid, u"var", read_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(var_data_txt_len, read_data.size());
	MEMCMP_EQUAL(var_data_txt, read_data.c_str(), var_data_txt_len);

	/* Try deleting the variable with old timestamp */
	status = m_client->set_variable(m_common_guid, u"var", VAR_delete_old_auth,
					sizeof(VAR_delete_old_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Delete variable */
	status = m_client->set_variable(m_common_guid, u"var", VAR_delete_auth,
					sizeof(VAR_delete_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Check if it doesn't exist */
	status = m_client->get_variable(m_common_guid, u"var", read_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);

	/* Set variable again */
	status = m_client->set_variable(m_common_guid, u"var", VAR_auth, sizeof(VAR_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Append variable with the old timestamp which should not modify the timestamp */
	status = m_client->set_variable(m_common_guid, u"var", VAR_append_old_auth, sizeof(VAR_append_old_auth),
					m_authentication_common_attributes | EFI_VARIABLE_APPEND_WRITE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Overwrite variable with the same timestamp. Should not succeed, because the timestamp was not compromised */
	status = m_client->set_variable(m_common_guid, u"var", VAR_auth, sizeof(VAR_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Delete variable */
	status = m_client->set_variable(m_common_guid, u"var", VAR_delete_auth,
					sizeof(VAR_delete_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
}

TEST(SmmVariableServiceTests, bootVarAuthenticationDisabled)
{
	efi_status_t status;
	std::string read_data;

	/* Without PK the authentication is disabled so each variable are writable, even in wrong order */
	status = m_client->set_variable(m_security_database_guid, u"db", (uint8_t *)DB1_auth,
					sizeof(DB1_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = m_client->set_variable(m_global_guid, u"KEK", (uint8_t *)KEK_auth,
					sizeof(KEK_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
}

TEST(SmmVariableServiceTests, bootVarAuthenticationSetAllKeys)
{
	efi_status_t status;

	/* Enable authentication via setting PK */
	status = m_client->set_variable(m_global_guid, u"PK", PK1_auth, sizeof(PK1_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Try setting db1 without KEK */
	status = m_client->set_variable(m_security_database_guid, u"db", DB1_auth,
					sizeof(DB1_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Set db2 that was signed by PK */
	status = m_client->set_variable(m_security_database_guid, u"db", DB2_auth,
					sizeof(DB2_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Set KEK */
	status = m_client->set_variable(m_global_guid, u"KEK", KEK_auth, sizeof(KEK_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Set db */
	status = m_client->set_variable(m_security_database_guid, u"db", DB1_auth,
					sizeof(DB1_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
}

TEST(SmmVariableServiceTests, bootVarAuthenticationRemove)
{
	efi_status_t status;

	/* Enable authentication via setting PK */
	status = m_client->set_variable(m_global_guid, u"PK", PK1_auth, sizeof(PK1_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Set KEK and db */
	status = m_client->set_variable(m_global_guid, u"KEK", KEK_auth, sizeof(KEK_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Remove KEK and try writing db with a valid request which should fail without KEK */
	status = m_client->set_variable(m_global_guid, u"KEK", KEK_delete_auth,
					sizeof(KEK_delete_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = m_client->set_variable(m_security_database_guid, u"db", DB1_auth,
					sizeof(DB1_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Try removing PK with an incorrect, non-authenticated delete request */
	status = m_client->remove_variable(m_global_guid, u"PK");
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Remove PK so now db can be overwritten, because authentication is disabled */
	status = m_client->set_variable(m_global_guid, u"PK", PK1_delete_auth,
					sizeof(PK1_delete_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = m_client->set_variable(m_security_database_guid, u"db", DB1_auth,
					sizeof(DB1_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
}

TEST(SmmVariableServiceTests, bootVarAuthenticationChangePK)
{
	efi_status_t status;

	/* Enable authentication via setting the platform key */
	status = m_client->set_variable(m_global_guid, u"PK", PK1_auth, sizeof(PK1_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* PK3 can not be set, because it is not signed by the current platform key */
	status = m_client->set_variable(m_global_guid, u"PK", PK3_auth, sizeof(PK3_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* PK2 can be set, because it is signed by the current platform key */
	status = m_client->set_variable(m_global_guid, u"PK", PK2_auth, sizeof(PK2_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Clear PK2 */
	status = m_client->set_variable(m_global_guid, u"PK", PK2_delete_auth,
					sizeof(PK2_delete_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
}

TEST(SmmVariableServiceTests, bootVarAuthenticationSignatureVerifyFailure)
{
	efi_status_t status;

	uint32_t dwLength = KEK_auth[16] | KEK_auth[17] << 8 | KEK_auth[18] << 16 |
			    KEK_auth[19] << 24;

	/* Find error injection points in signature, public key and timestamp fields */
	uint8_t *signature = &KEK_auth[dwLength + 16 - 1];
	uint8_t *public_key = &KEK_auth[sizeof(KEK_auth) - 1];
	uint8_t *timestamp = &KEK_auth[0];

	/* Enable authentication via setting PK */
	status = m_client->set_variable(m_global_guid, u"PK", PK1_auth, sizeof(PK1_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Try setting KEK with wrong signature */
	*signature = ~(*signature);
	status = m_client->set_variable(m_global_guid, u"KEK", KEK_auth, sizeof(KEK_auth),
					m_authentication_common_attributes);
	*signature = ~(*signature);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Try setting KEK with wrong public key */
	*public_key = ~(*public_key);
	status = m_client->set_variable(m_global_guid, u"KEK", KEK_auth, sizeof(KEK_auth),
					m_authentication_common_attributes);
	*public_key = ~(*public_key);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Try setting KEK with wrong timestamp that results in wrong hash */
	*timestamp = ~(*timestamp);
	status = m_client->set_variable(m_global_guid, u"KEK", KEK_auth, sizeof(KEK_auth),
					m_authentication_common_attributes);
	*timestamp = ~(*timestamp);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* db can not be set, because KEK was not added to the store */
	status = m_client->set_variable(m_security_database_guid, u"db", DB1_auth,
					sizeof(DB1_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SECURITY_VIOLATION, status);

	/* Set correct KEK */
	status = m_client->set_variable(m_global_guid, u"KEK", KEK_auth, sizeof(KEK_auth),
					m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Set db */
	status = m_client->set_variable(m_security_database_guid, u"db", DB1_auth,
					sizeof(DB1_auth), m_authentication_common_attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
}
#endif
