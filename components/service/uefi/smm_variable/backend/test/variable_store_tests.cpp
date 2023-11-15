/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <service/secure_storage/backend/mock_store/mock_store.h>
#include <service/uefi/smm_variable/backend/uefi_variable_store.h>
#include <string.h>
#include <string>
#include <vector>

TEST_GROUP(UefiVariableStoreTests)
{
	void setup()
	{
		m_persistent_backend = mock_store_init(&m_persistent_store);
		m_volatile_backend = mock_store_init(&m_volatile_store);

		efi_status_t status = uefi_variable_store_init(&m_uefi_variable_store, OWNER_ID,
							       MAX_VARIABLES, m_persistent_backend,
							       m_volatile_backend);

		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

		uefi_variable_store_set_storage_limits(&m_uefi_variable_store,
						       EFI_VARIABLE_NON_VOLATILE, STORE_CAPACITY,
						       MAX_VARIABLE_SIZE);

		uefi_variable_store_set_storage_limits(&m_uefi_variable_store, 0, STORE_CAPACITY,
						       MAX_VARIABLE_SIZE);

		setup_common_guid();
	}

	void teardown()
	{
		uefi_variable_store_deinit(&m_uefi_variable_store);
		mock_store_reset(&m_persistent_store);
		mock_store_reset(&m_volatile_store);
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

	std::vector<int16_t> to_variable_name(const std::wstring &string)
	{
		std::vector<int16_t> var_name;

		for (size_t i = 0; i < string.size(); i++) {
			var_name.push_back((int16_t)string[i]);
		}

		/* Add mandatory null terminator */
		var_name.push_back(0);

		return var_name;
	}

	bool compare_variable_name(const std::wstring &expected, const int16_t *name,
				   size_t name_size)
	{
		bool is_equal = (expected.size() + 1 <= name_size / sizeof(int16_t));

		for (size_t i = 0; is_equal && i < expected.size(); i++) {
			if (name[i] != (int16_t)expected[i]) {
				is_equal = false;
				break;
			}
		}

		return is_equal;
	}

	efi_status_t set_variable(const std::wstring &name, const std::string &data,
				  uint32_t attributes)
	{
		std::vector<int16_t> var_name = to_variable_name(name);
		size_t name_size = var_name.size() * sizeof(int16_t);
		size_t data_size = data.size();
		uint8_t msg_buffer[SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_SIZE(name_size,
										 data_size)];

		SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *access_variable =
			(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)msg_buffer;

		access_variable->Guid = m_common_guid;
		access_variable->Attributes = attributes;

		access_variable->NameSize = name_size;
		memcpy(access_variable->Name, var_name.data(), name_size);

		access_variable->DataSize = data_size;
		memcpy(&msg_buffer[SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(
			       access_variable)],
		       data.c_str(), data_size);

		efi_status_t status =
			uefi_variable_store_set_variable(&m_uefi_variable_store, access_variable);

		return status;
	}

	efi_status_t get_variable(const std::wstring &name, std::string &data,
				  size_t data_len_clamp = VARIABLE_BUFFER_SIZE)
	{
		std::vector<int16_t> var_name = to_variable_name(name);
		size_t name_size = var_name.size() * sizeof(int16_t);
		size_t total_size = 0;
		uint8_t msg_buffer[VARIABLE_BUFFER_SIZE];

		SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *access_variable =
			(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)msg_buffer;

		access_variable->Guid = m_common_guid;
		access_variable->Attributes = 0;

		access_variable->NameSize = name_size;
		memcpy(access_variable->Name, var_name.data(), name_size);

		size_t max_data_len =
			(data_len_clamp == VARIABLE_BUFFER_SIZE) ?
				VARIABLE_BUFFER_SIZE -
					SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(
						access_variable) :
				data_len_clamp;

		access_variable->DataSize = max_data_len;

		efi_status_t status = uefi_variable_store_get_variable(
			&m_uefi_variable_store, access_variable, max_data_len, &total_size);

		data.clear();

		if (status == EFI_SUCCESS) {
			const char *data_start =
				(const char *)(msg_buffer +
					       SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(
						       access_variable));

			data = std::string(data_start, access_variable->DataSize);

			UNSIGNED_LONGLONGS_EQUAL(
				SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_TOTAL_SIZE(
					access_variable),
				total_size);
		} else if (status == EFI_BUFFER_TOO_SMALL) {
			/* String length set to reported variable length */
			data.insert(0, access_variable->DataSize, '!');

			UNSIGNED_LONGLONGS_EQUAL(
				SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(
					access_variable),
				total_size);
		}

		return status;
	}

	efi_status_t query_variable_info(uint32_t attributes, size_t * max_variable_storage_size,
					 size_t * remaining_variable_storage_size,
					 size_t * max_variable_size)
	{
		SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO query;

		query.MaximumVariableStorageSize = 0;
		query.RemainingVariableStorageSize = 0;
		query.MaximumVariableSize = 0;
		query.Attributes = attributes;

		efi_status_t status =
			uefi_variable_store_query_variable_info(&m_uefi_variable_store, &query);

		if (status == EFI_SUCCESS) {
			*max_variable_storage_size = query.MaximumVariableStorageSize;
			*remaining_variable_storage_size = query.RemainingVariableStorageSize;
			*max_variable_size = query.MaximumVariableSize;
		}

		return status;
	}

	efi_status_t set_check_var_property(const std::wstring &name,
					    const VAR_CHECK_VARIABLE_PROPERTY &check_property)
	{
		std::vector<int16_t> var_name = to_variable_name(name);
		size_t name_size = var_name.size() * sizeof(int16_t);
		uint8_t msg_buffer[SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_SIZE(
			name_size)];

		SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *check_var =
			(SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)msg_buffer;

		check_var->Guid = m_common_guid;
		check_var->NameSize = name_size;
		memcpy(check_var->Name, var_name.data(), name_size);

		check_var->VariableProperty = check_property;

		efi_status_t status = uefi_variable_store_set_var_check_property(
			&m_uefi_variable_store, check_var);

		return status;
	}

	void zap_stored_variable(const std::wstring &name)
	{
		std::vector<int16_t> var_name = to_variable_name(name);
		size_t name_size = var_name.size() * sizeof(int16_t);

		/* Create the condition where a variable is indexed but
		 * there is no corresponding stored object.
		 */
		struct variable_index *variable_index = &m_uefi_variable_store.variable_index;

		const struct variable_info *info = variable_index_find(
			variable_index, &m_common_guid, name_size, var_name.data());

		if (info && (info->metadata.attributes & EFI_VARIABLE_NON_VOLATILE)) {
			struct storage_backend *storage_backend =
				m_uefi_variable_store.persistent_store.storage_backend;

			storage_backend->interface->remove(storage_backend->context, OWNER_ID,
							   info->metadata.uid);
		}
	}

	void power_cycle()
	{
		/* Simulate a power-cycle */
		uefi_variable_store_deinit(&m_uefi_variable_store);

		/* Lose volatile store contents */
		mock_store_reset(&m_volatile_store);

		efi_status_t status = uefi_variable_store_init(&m_uefi_variable_store, OWNER_ID,
							       MAX_VARIABLES, m_persistent_backend,
							       m_volatile_backend);

		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

		uefi_variable_store_set_storage_limits(&m_uefi_variable_store,
						       EFI_VARIABLE_NON_VOLATILE, STORE_CAPACITY,
						       MAX_VARIABLE_SIZE);

		uefi_variable_store_set_storage_limits(&m_uefi_variable_store, 0, STORE_CAPACITY,
						       MAX_VARIABLE_SIZE);
	}

	static const size_t MAX_VARIABLES = 10;
	static const size_t MAX_VARIABLE_SIZE = 100;
	static const size_t STORE_CAPACITY = 1000;

	static const uint32_t OWNER_ID = 100;
	static const size_t VARIABLE_BUFFER_SIZE = 1024;

	struct uefi_variable_store m_uefi_variable_store;
	struct mock_store m_persistent_store;
	struct mock_store m_volatile_store;
	struct storage_backend *m_persistent_backend;
	struct storage_backend *m_volatile_backend;
	EFI_GUID m_common_guid;
};

TEST(UefiVariableStoreTests, setGetRoundtrip)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name = L"test_variable";
	std::string input_data = "quick brown fox";
	std::string output_data;

	status = set_variable(var_name, input_data, 0);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect got variable data to be the same as the set value */
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
	LONGS_EQUAL(0, input_data.compare(output_data));

	/* Extend the variable using an append write */
	std::string input_data2 = " jumps over the lazy dog";

	status = set_variable(var_name, input_data2, EFI_VARIABLE_APPEND_WRITE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	std::string expected_output = input_data + input_data2;

	/* Expect the append write operation to have extended the variable */
	UNSIGNED_LONGLONGS_EQUAL(expected_output.size(), output_data.size());
	LONGS_EQUAL(0, expected_output.compare(output_data));

	/* Expect query_variable_info to return consistent values */
	size_t max_variable_storage_size = 0;
	size_t remaining_variable_storage_size = 0;
	size_t max_variable_size = 0;

	status = query_variable_info(0, &max_variable_storage_size,
				     &remaining_variable_storage_size, &max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	UNSIGNED_LONGLONGS_EQUAL(STORE_CAPACITY, max_variable_storage_size);
	UNSIGNED_LONGLONGS_EQUAL(MAX_VARIABLE_SIZE, max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(STORE_CAPACITY - expected_output.size(),
				 remaining_variable_storage_size);
}

TEST(UefiVariableStoreTests, persistentSetGet)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name = L"test_variable";
	std::string input_data = "quick brown fox";
	std::string output_data;

	status = set_variable(var_name, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect got variable data to be the same as the set value */
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
	LONGS_EQUAL(0, input_data.compare(output_data));

	/* Extend the variable using an append write */
	std::string input_data2 = " jumps over the lazy dog";

	status = set_variable(var_name, input_data2,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_APPEND_WRITE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	std::string expected_output = input_data + input_data2;

	/* Expect the append write operation to have extended the variable */
	UNSIGNED_LONGLONGS_EQUAL(expected_output.size(), output_data.size());
	LONGS_EQUAL(0, expected_output.compare(output_data));

	/* Expect the variable to survive a power cycle */
	power_cycle();

	output_data = std::string();
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Still expect got variable data to be the same as the set value */
	UNSIGNED_LONGLONGS_EQUAL(expected_output.size(), output_data.size());
	LONGS_EQUAL(0, expected_output.compare(output_data));

	/* Expect query_variable_info to return consistent values */
	size_t max_variable_storage_size = 0;
	size_t remaining_variable_storage_size = 0;
	size_t max_variable_size = 0;

	status = query_variable_info(EFI_VARIABLE_NON_VOLATILE, &max_variable_storage_size,
				     &remaining_variable_storage_size, &max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	UNSIGNED_LONGLONGS_EQUAL(STORE_CAPACITY, max_variable_storage_size);
	UNSIGNED_LONGLONGS_EQUAL(MAX_VARIABLE_SIZE, max_variable_size);
	UNSIGNED_LONGLONGS_EQUAL(STORE_CAPACITY - expected_output.size(),
				 remaining_variable_storage_size);
}

TEST(UefiVariableStoreTests, getWithSmallBuffer)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name = L"test_variable";
	std::string input_data = "quick brown fox";
	std::string output_data;

	/* A get with a zero length buffer is a legitimate way to
	 * discover the variable size. This test performs GetVariable
	 * operations with various buffer small buffer sizes. */
	status = set_variable(var_name, input_data, 0);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* First get the variable without a constrained buffer */
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect got variable data to be the same as the set value */
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
	LONGS_EQUAL(0, input_data.compare(output_data));

	/* Now try with a zero length buffer */
	status = get_variable(var_name, output_data, 0);
	UNSIGNED_LONGLONGS_EQUAL(EFI_BUFFER_TOO_SMALL, status);
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());

	/* Try with a non-zero length but too small buffer */
	status = get_variable(var_name, output_data, input_data.size() - 1);
	UNSIGNED_LONGLONGS_EQUAL(EFI_BUFFER_TOO_SMALL, status);
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
}

TEST(UefiVariableStoreTests, removeVolatile)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name = L"rm_volatile_variable";
	std::string input_data = "quick brown fox";
	std::string output_data;

	status = set_variable(var_name, input_data, 0);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Remove by setting with zero data length */
	status = set_variable(var_name, std::string(), 0);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect variable to no loger exist */
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableStoreTests, removePersistent)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name = L"rm_nv_variable";
	std::string input_data = "quick brown fox";
	std::string output_data;

	/* Attempt to remove a non-existed variable */
	status = set_variable(var_name, std::string(), EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);

	/* Create a variable */
	status = set_variable(var_name, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Remove by setting with zero data length */
	status = set_variable(var_name, std::string(), EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect variable to no loger exist */
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableStoreTests, bootServiceAccess)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name = L"test_variable";
	std::string input_data = "a variable with access restricted to boot";
	std::string output_data;

	status = set_variable(var_name, input_data,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* 'Reboot' */
	power_cycle();

	/* Expect access to be permitted */
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
	LONGS_EQUAL(0, input_data.compare(output_data));

	/* End of boot phase */
	status = uefi_variable_store_exit_boot_service(&m_uefi_variable_store);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect access to be blocked */
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_ACCESS_DENIED, status);
}

TEST(UefiVariableStoreTests, runtimeAccess)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name = L"test_variable";
	std::string input_data = "a variable with access restricted to runtime";
	std::string output_data;

	/*
	 * Client is reponsible for setting bootservice access whenever runtime
	 * access is specified. This checks the defence against invalid attributes.
	 */
	status = set_variable(var_name, input_data,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, status);

	status = set_variable(var_name, input_data,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
				      EFI_VARIABLE_BOOTSERVICE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* 'Reboot' */
	power_cycle();

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* End of boot phase */
	status = uefi_variable_store_exit_boot_service(&m_uefi_variable_store);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect access to be permitted */
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
	LONGS_EQUAL(0, input_data.compare(output_data));
}

TEST(UefiVariableStoreTests, enumerateStoreContents)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name_1 = L"test_variable_1";
	std::wstring var_name_2 = L"test_variable_2";
	std::wstring var_name_3 = L"test_variable_3";
	std::string input_data = "blah blah";

	/* Add some variables - a mixture of NV and volatile */
	status = set_variable(var_name_1, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = set_variable(var_name_2, input_data, 0);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = set_variable(var_name_3, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Prepare to enumerate */
	size_t total_len = 0;
	uint8_t msg_buffer[VARIABLE_BUFFER_SIZE];
	SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *next_name =
		(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *)msg_buffer;
	size_t max_name_len =
		VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;

	/* First check handling of invalid variable name */
	std::vector<int16_t> bogus_name = to_variable_name(L"bogus_variable");
	size_t bogus_name_size = bogus_name.size() * sizeof(uint16_t);
	next_name->Guid = m_common_guid;
	next_name->NameSize = bogus_name_size;
	memcpy(next_name->Name, bogus_name.data(), bogus_name_size);

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, status);

	/* Enumerate store contents */
	next_name->NameSize = sizeof(int16_t);
	next_name->Name[0] = 0;

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_1, next_name->Name, next_name->NameSize));

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_2, next_name->Name, next_name->NameSize));

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_3, next_name->Name, next_name->NameSize));

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);

	power_cycle();

	/* Enumerate again - should be left with just NV variables.
	 * Use a different but equally valid null name.
	 */
	next_name->NameSize = 10 * sizeof(int16_t);
	memset(next_name->Name, 0, next_name->NameSize);

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_1, next_name->Name, next_name->NameSize));

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_3, next_name->Name, next_name->NameSize));

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableStoreTests, failedNvSet)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name_1 = L"test_variable_1";
	std::wstring var_name_2 = L"test_variable_2";
	std::wstring var_name_3 = L"test_variable_3";
	std::string input_data = "blah blah";

	/* Add some variables - a mixture of NV and volatile */
	status = set_variable(var_name_1, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = set_variable(var_name_2, input_data, 0);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = set_variable(var_name_3, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Simulate a power failure which resulted in the
	 * variable index being written but not the corresponding
	 * data.
	 */
	zap_stored_variable(var_name_3);
	power_cycle();

	/* After the power cycle, we expect the volatile variable
	 * to have gone and for the index to have been cleaned up
	 * for the failed variable 3.
	 */
	uint8_t msg_buffer[VARIABLE_BUFFER_SIZE];
	SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *next_name =
		(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *)msg_buffer;
	size_t max_name_len =
		VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;

	/* Enumerate store contents */
	size_t total_len = 0;
	next_name->NameSize = sizeof(int16_t);
	next_name->Name[0] = 0;

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_1, next_name->Name, next_name->NameSize));

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    max_name_len, &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableStoreTests, unsupportedAttribute)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name_1 = L"test_variable_1";
	std::string input_data = "blah blah";

	/* Add a variable with an unsupported attribute */
	status = set_variable(var_name_1, input_data,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_UNSUPPORTED, status);
}

TEST(UefiVariableStoreTests, readOnlycheck)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name_1 = L"test_variable_1";
	std::string input_data = "blah blah";

	/* Add a variable */
	status = set_variable(var_name_1, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Apply a check to constrain to Read Only */
	VAR_CHECK_VARIABLE_PROPERTY check_property;
	check_property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
	check_property.Attributes = 0;
	check_property.Property = VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
	check_property.MinSize = 0;
	check_property.MaxSize = 100;

	status = set_check_var_property(var_name_1, check_property);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Subsequent set operations should fail */
	status = set_variable(var_name_1, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_WRITE_PROTECTED, status);
}

TEST(UefiVariableStoreTests, noRemoveCheck)
{
	efi_status_t status = EFI_SUCCESS;
	std::wstring var_name_1 = L"test_variable_1";
	std::string input_data = "blah blah";

	/* Add a variable */
	status = set_variable(var_name_1, input_data, EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Apply a check to constrain size to > 0.  This should prevent removal */
	VAR_CHECK_VARIABLE_PROPERTY check_property;
	check_property.Revision = VAR_CHECK_VARIABLE_PROPERTY_REVISION;
	check_property.Attributes = 0;
	check_property.Property = 0;
	check_property.MinSize = 1;
	check_property.MaxSize = 10;

	status = set_check_var_property(var_name_1, check_property);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Try and remove by setting with zero length data */
	status = set_variable(var_name_1, std::string(), EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, status);

	/* Setting with non zero data should work */
	status = set_variable(var_name_1, std::string("Good"), EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* But with data that exceeds the MaxSize */
	status = set_variable(var_name_1, std::string("A data value that exceeds the MaxSize"),
			      EFI_VARIABLE_NON_VOLATILE);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, status);
}
