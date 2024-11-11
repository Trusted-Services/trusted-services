/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <limits>
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
	}

	void teardown()
	{
		uefi_variable_store_deinit(&m_uefi_variable_store);
		mock_store_reset(&m_persistent_store);
		mock_store_reset(&m_volatile_store);
	}

	std::u16string to_variable_name(const std::u16string &string)
	{
		std::u16string var_name(string);

		/* Add mandatory null terminator */
		var_name.push_back(0);

		return var_name;
	}

	std::u16string to_variable_name(const char16_t *string)
	{
		std::u16string var_name(string);
		var_name.push_back(0);
		return var_name;
	}

	std::u16string intToChar16(const int i)
	{
		auto s = std::to_string(i);
		return { s.begin(), s.end() };
	}

	size_t string_get_size_in_bytes(const std::u16string &string)
	{
		return string.size() * sizeof(uint16_t);
	}

	bool compare_variable_name(const std::u16string &expected, const int16_t *name,
				   size_t name_size)
	{
		std::u16string var_name = to_variable_name(expected);

		if (name_size != string_get_size_in_bytes(var_name))
			return false;

		return 0==memcmp(name, var_name.data(), name_size);
	}

	efi_status_t set_variable(const std::u16string &name, const uint8_t *data, size_t data_size,
				  uint32_t attributes, EFI_GUID *guid = NULL)
	{
		std::u16string var_name = to_variable_name(name);
		size_t name_size = string_get_size_in_bytes(var_name);

		/* Use a vector as a temporary buffer to move allocation to the HEAP and for RAII benefits. */
		std::vector<uint8_t> msg_buffer(
			(std::size_t)SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_SIZE(name_size, data_size));

		SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *access_variable =
			(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *) msg_buffer.data();

		if (!guid)
			guid = &m_common_guid;
		access_variable->Guid = *guid;
		access_variable->Attributes = attributes;

		access_variable->NameSize = name_size;
		memcpy(access_variable->Name, var_name.data(), name_size);

		access_variable->DataSize = data_size;
		memcpy(msg_buffer.data() +
			SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(access_variable),
			data, data_size);

		efi_status_t status =
			uefi_variable_store_set_variable(&m_uefi_variable_store, access_variable);

		return status;
	}

	efi_status_t set_variable(const std::u16string &name, const std::string &data,
				  uint32_t attributes, EFI_GUID *guid = NULL)
	{
		return set_variable(name, (uint8_t *) data.data(), data.size(),
					attributes, guid);
	}

	efi_status_t get_variable(const std::u16string &name, std::string &data,
				  size_t data_len_clamp = VARIABLE_BUFFER_SIZE)
	{
		std::u16string var_name = to_variable_name(name);
		size_t name_size = string_get_size_in_bytes(var_name);

		size_t total_size = 0;
		std::vector<uint8_t> msg_buffer(VARIABLE_BUFFER_SIZE);

		SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *access_variable =
			(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *) msg_buffer.data();

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
				(const char *) (msg_buffer.data() +
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

	efi_status_t set_check_var_property(const std::u16string &name,
					    const VAR_CHECK_VARIABLE_PROPERTY &check_property)
	{
		std::u16string var_name = to_variable_name(name);
		size_t name_size = string_get_size_in_bytes(var_name);

		std::vector<uint8_t> msg_buffer(
			SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_SIZE(name_size));

		SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *check_var =
			(SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *) msg_buffer.data();

		check_var->Guid = m_common_guid;
		check_var->NameSize = name_size;
		memcpy(check_var->Name, var_name.data(), name_size);

		check_var->VariableProperty = check_property;

		efi_status_t status = uefi_variable_store_set_var_check_property(
			&m_uefi_variable_store, check_var);

		return status;
	}

	void zap_stored_variable(const std::u16string &name)
	{
		std::u16string var_name = to_variable_name(name);
		size_t name_size = string_get_size_in_bytes(var_name);

		/* Create the condition where a variable is indexed but
		 * there is no corresponding stored object.
		 */
		struct variable_index *variable_index = &m_uefi_variable_store.variable_index;

		const struct variable_info *info = variable_index_find(
			variable_index, &m_common_guid, name_size,
			(const int16_t *) var_name.data());

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

	static const size_t MAX_VARIABLES = 5;
	static const size_t MAX_VARIABLE_SIZE = 3000;
	static const size_t STORE_CAPACITY = MAX_VARIABLES * MAX_VARIABLE_SIZE;
	static const size_t VARIABLE_INDEX_MAX_SIZE =
		sizeof(uint32_t) +
		MAX_VARIABLES * (sizeof(struct variable_metadata) +
				 sizeof(struct variable_constraints) + sizeof(bool));

	static const uint32_t OWNER_ID = 100;

	/* Synchronize these with the variables with the store */
	uint64_t DEFAULT_VARIABLE_INDEX_STORAGE_A_UID = 0x8000000000000001;
	uint64_t DEFAULT_VARIABLE_INDEX_STORAGE_B_UID = 0x8000000000000002;

	/*
	 * Make sure the variable buffer in the test is way above the limit
	 * so the buffer problems will be handled by the component
	 * under test.
	 */
	static const size_t VARIABLE_BUFFER_SIZE = MAX_VARIABLE_SIZE * 2;

	struct uefi_variable_store m_uefi_variable_store;
	struct mock_store m_persistent_store;
	struct mock_store m_volatile_store;
	struct storage_backend *m_persistent_backend;
	struct storage_backend *m_volatile_backend;

	EFI_GUID m_common_guid = { 0x01234567,
				   0x89ab,
				   0xCDEF,
				   { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef } };
};

TEST(UefiVariableStoreTests, setGetRoundtrip)
{
	efi_status_t status = EFI_SUCCESS;
	std::u16string var_name = u"test_variable";
	std::string input_data = "quick brown fox";
	std::string output_data;

	status = set_variable(var_name, input_data, EFI_VARIABLE_BOOTSERVICE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect got variable data to be the same as the set value */
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
	LONGS_EQUAL(0, input_data.compare(output_data));

	/* Extend the variable using an append write */
	std::string input_data2 = " jumps over the lazy dog";

	status = set_variable(var_name, input_data2,
			      EFI_VARIABLE_APPEND_WRITE | EFI_VARIABLE_BOOTSERVICE_ACCESS);
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
	std::u16string var_name = u"test_variable";
	std::string input_data = "quick brown fox";
	std::string output_data;

	status = set_variable(var_name, input_data,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Expect got variable data to be the same as the set value */
	UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
	LONGS_EQUAL(0, input_data.compare(output_data));

	/* Extend the variable using an append write */
	std::string input_data2 = " jumps over the lazy dog";

	status = set_variable(var_name, input_data2,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_APPEND_WRITE |
				      EFI_VARIABLE_BOOTSERVICE_ACCESS);
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
	std::u16string var_name = u"test_variable";
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
	std::u16string var_name = u"rm_volatile_variable";
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
	std::u16string var_name = u"rm_nv_variable";
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
	std::u16string var_name = u"test_variable";
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

	/* Expect access to be blocked for get_variable */
	status = get_variable(var_name, output_data);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);

	/* Expect access to be blocked for get_next_variable_name */
	std::vector<uint8_t> msg_buffer(VARIABLE_BUFFER_SIZE);
	SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *next_name =
		(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *) msg_buffer.data();

	size_t total_len = 0;
	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	next_name->Name[0] = 0;

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);

	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableStoreTests, runtimeAccess)
{
	efi_status_t status = EFI_SUCCESS;
	std::u16string var_name = u"test_variable";
	std::string input_data = "a variable with access restricted to runtime";
	std::string output_data;

	/*
	 * Client is responsible for setting bootservice access whenever runtime
	 * access is specified. This checks the defense against invalid attributes.
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
	std::u16string var_name_1 = u"test_variable_1";
	std::u16string var_name_2 = u"test_variable_2";
	std::u16string var_name_3 = u"test_variable_3";
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
	std::vector<uint8_t> msg_buffer(VARIABLE_BUFFER_SIZE);
	SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *next_name =
		(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *) msg_buffer.data();

	/* First check handling of invalid variable name */
	std::u16string bogus_name = to_variable_name(u"bogus_variable");
	size_t bogus_name_size = string_get_size_in_bytes(bogus_name);
	next_name->Guid = m_common_guid;
	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	memcpy(next_name->Name, bogus_name.data(), bogus_name_size);

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, status);

	/* Enumerate store contents */
	next_name->NameSize = sizeof(int16_t);
	next_name->Name[0] = 0;
	/* Check if the correct NameSize is returned if namesize is too small */
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_BUFFER_TOO_SMALL, status);
	UNSIGNED_LONGLONGS_EQUAL(sizeof(var_name_1), next_name->NameSize);

	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_1, next_name->Name, next_name->NameSize));

	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_2, next_name->Name, next_name->NameSize));

	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_3, next_name->Name, next_name->NameSize));

	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);

	power_cycle();

	/* Enumerate again - should be left with just NV variables.
	 * Use a different but equally valid null name.
	 */
	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	memset(next_name->Name, 0, next_name->NameSize);

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_1, next_name->Name, next_name->NameSize));

	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_3, next_name->Name, next_name->NameSize));

	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableStoreTests, failedNvSet)
{
	efi_status_t status = EFI_SUCCESS;
	std::u16string var_name_1 = u"test_variable_1";
	std::u16string var_name_2 = u"test_variable_2";
	std::u16string var_name_3 = u"test_variable_3";
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
	std::vector<uint8_t> msg_buffer(VARIABLE_BUFFER_SIZE);
	SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *next_name =
		(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *) msg_buffer.data();

	/* Enumerate store contents */
	size_t total_len = 0;
	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	next_name->Name[0] = 0;

	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(compare_variable_name(var_name_1, next_name->Name, next_name->NameSize));

	next_name->NameSize = VARIABLE_BUFFER_SIZE - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;
	status = uefi_variable_store_get_next_variable_name(&m_uefi_variable_store, next_name,
							    &total_len);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableStoreTests, unsupportedAttribute)
{
	efi_status_t status = EFI_SUCCESS;
	std::u16string var_name_1 = u"test_variable_1";
	std::string input_data = "blah blah";

	/* Add a variable with an unsupported attribute */
	status = set_variable(var_name_1, input_data,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_UNSUPPORTED, status);
}

TEST(UefiVariableStoreTests, readOnlycheck)
{
	efi_status_t status = EFI_SUCCESS;
	std::u16string var_name_1 = u"test_variable_1";
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
	std::u16string var_name_1 = u"test_variable_1";
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

TEST(UefiVariableStoreTests, fillStore)
{
	efi_status_t status = EFI_SUCCESS;

	/* Fill the variable store with max size variables */
	for (size_t i = 0; i < MAX_VARIABLES; i++) {
		std::u16string current_var = u"var_";
		std::string input_data(MAX_VARIABLE_SIZE, 'a');
		std::string output_data;
		current_var += intToChar16(i);

		status = set_variable(current_var, input_data,
				      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
					      EFI_VARIABLE_RUNTIME_ACCESS);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

		/* Verify the write */
		status = get_variable(current_var, output_data);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

		/* Expect got variable data to be the same as the set value */
		UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
		LONGS_EQUAL(0, input_data.compare(output_data));
	}

	/* Try adding a small variable to an already full store */
	status = set_variable(u"var", "a",
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
				      EFI_VARIABLE_RUNTIME_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_OUT_OF_RESOURCES, status);
}

TEST(UefiVariableStoreTests, fillIndex)
{
	efi_status_t status = EFI_SUCCESS;
	std::u16string var_name = u"var";
	std::string input_data = "a";
	std::string output_data;

	/*
	 * Fill the variable store with small variables so the index
	 * will be filled, but the store does not
	 */
	for (size_t i = 0; i < MAX_VARIABLES; i++) {
		std::u16string current_var = u"var_";
		current_var += intToChar16(i);

		status = set_variable(current_var, input_data,
				      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
					      EFI_VARIABLE_RUNTIME_ACCESS);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

		/* Verify the write */
		status = get_variable(current_var, output_data);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

		/* Expect got variable data to be the same as the set value */
		UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
		LONGS_EQUAL(0, input_data.compare(output_data));
	}

	/* Try adding a small variable to an already full store */
	input_data.resize(1, 'a');

	status = set_variable(u"var", input_data,
			      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
				      EFI_VARIABLE_RUNTIME_ACCESS);
	UNSIGNED_LONGLONGS_EQUAL(EFI_OUT_OF_RESOURCES, status);

	/* Simulate a power-cycle without deleting the NV store content */
	uefi_variable_store_deinit(&m_uefi_variable_store);

	/* Try loading the non-volatile variables */
	status = uefi_variable_store_init(&m_uefi_variable_store, OWNER_ID, MAX_VARIABLES,
					  m_persistent_backend, m_volatile_backend);

	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Try reading the previously set variables */
	for (size_t i = 0; i < MAX_VARIABLES; i++) {
		std::u16string current_var = u"var_";
		current_var += intToChar16(i);

		status = get_variable(current_var, output_data);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
		UNSIGNED_LONGLONGS_EQUAL(input_data.size(), output_data.size());
		LONGS_EQUAL(0, input_data.compare(output_data));
	}
}

TEST(UefiVariableStoreTests, variableIndexCounterOverflow)
{
	efi_status_t efi_status = EFI_SUCCESS;
	psa_status_t psa_status = PSA_SUCCESS;
	std::u16string var_name = u"var";
	std::string input_data = "a";
	uint32_t attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
			      EFI_VARIABLE_RUNTIME_ACCESS;
	/* There are no variables set in the index, only the counter is there */
	uint8_t buffer[sizeof(uint32_t)] = { 0 };

	mock_store_reset(&m_persistent_store);

	/* Counter of index A is 0 */
	psa_status = m_persistent_store.backend.interface->set(
		m_persistent_store.backend.context, OWNER_ID, DEFAULT_VARIABLE_INDEX_STORAGE_A_UID,
		sizeof(buffer), &buffer, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, psa_status);

	/* Set max counter value */
	buffer[0] = 0xFF;
	buffer[1] = 0xFF;
	buffer[2] = 0xFF;
	buffer[3] = 0xFF;

	/* Counter of index B is max value */
	psa_status = m_persistent_store.backend.interface->set(
		m_persistent_store.backend.context, OWNER_ID, DEFAULT_VARIABLE_INDEX_STORAGE_B_UID,
		sizeof(buffer), &buffer, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, psa_status);

	/* At next initialization of the store index A should be the latest index with counter value 0 */
	uefi_variable_store_deinit(&m_uefi_variable_store);

	efi_status = uefi_variable_store_init(&m_uefi_variable_store, OWNER_ID, MAX_VARIABLES,
					      m_persistent_backend, m_volatile_backend);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.active_variable_index_uid,
				 DEFAULT_VARIABLE_INDEX_STORAGE_A_UID);
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.variable_index.counter, 0);

	/* After setting a variable to trigger sync and rebooting index B should be the latest index with counter value 1*/
	efi_status = set_variable(var_name, input_data, attributes);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, efi_status);

	power_cycle();

	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.active_variable_index_uid,
				 DEFAULT_VARIABLE_INDEX_STORAGE_B_UID);
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.variable_index.counter, 1);
}

TEST(UefiVariableStoreTests, oneEmptyVariableIndexExists)
{
	psa_status_t status = PSA_SUCCESS;

	/* Only, variable index A exists, but it is empty */
	mock_store_reset(&m_persistent_store);

	status = m_persistent_store.backend.interface->create(m_persistent_store.backend.context,
							      OWNER_ID,
							      DEFAULT_VARIABLE_INDEX_STORAGE_A_UID,
							      100, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, status);

	power_cycle();

	/* Empty index is considered non-existing so default index (A) is selected */
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.active_variable_index_uid,
				 DEFAULT_VARIABLE_INDEX_STORAGE_A_UID);
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.variable_index.counter, 0);

	/* Only, variable index B exists, but it is empty*/
	mock_store_reset(&m_persistent_store);

	status = m_persistent_store.backend.interface->create(m_persistent_store.backend.context,
							      OWNER_ID,
							      DEFAULT_VARIABLE_INDEX_STORAGE_B_UID,
							      100, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, status);

	power_cycle();

	/* Empty index is considered non-existing so default index (A) is selected */
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.active_variable_index_uid,
				 DEFAULT_VARIABLE_INDEX_STORAGE_A_UID);
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.variable_index.counter, 0);
}

TEST(UefiVariableStoreTests, oneVariableIndexAlreadySet)
{
	efi_status_t status = EFI_SUCCESS;
	/* Empty variable index with zero counter value */
	uint8_t buffer[VARIABLE_INDEX_MAX_SIZE] = { 0 };

	/* Set index A in the store with some data, so it will be found as the currently active index */
	mock_store_reset(&m_persistent_store);

	status = m_persistent_store.backend.interface->set(
		m_persistent_store.backend.context, OWNER_ID, DEFAULT_VARIABLE_INDEX_STORAGE_A_UID,
		sizeof(buffer), &buffer, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, status);

	power_cycle();

	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.active_variable_index_uid,
				 DEFAULT_VARIABLE_INDEX_STORAGE_A_UID);
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.variable_index.counter, 0);

	/* Set index B in the store with some data, so it will be found as the currently active index */
	mock_store_reset(&m_persistent_store);

	status = m_persistent_store.backend.interface->set(
		m_persistent_store.backend.context, OWNER_ID, DEFAULT_VARIABLE_INDEX_STORAGE_B_UID,
		sizeof(buffer), &buffer, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, status);

	power_cycle();

	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.active_variable_index_uid,
				 DEFAULT_VARIABLE_INDEX_STORAGE_B_UID);
	UNSIGNED_LONGLONGS_EQUAL(m_uefi_variable_store.variable_index.counter, 0);
}

TEST(UefiVariableStoreTests, variableIndexesWithSameData)
{
	psa_status_t psa_status = PSA_SUCCESS;
	efi_status_t efi_status = EFI_SUCCESS;
	/* Empty variable index with zero counter value */
	uint8_t buffer[VARIABLE_INDEX_MAX_SIZE] = { 0 };

	/* Set both indexes to the same data and counter value */
	mock_store_reset(&m_persistent_store);

	psa_status = m_persistent_store.backend.interface->set(
		m_persistent_store.backend.context, OWNER_ID, DEFAULT_VARIABLE_INDEX_STORAGE_A_UID,
		sizeof(buffer), &buffer, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, psa_status);

	psa_status = m_persistent_store.backend.interface->set(
		m_persistent_store.backend.context, OWNER_ID, DEFAULT_VARIABLE_INDEX_STORAGE_B_UID,
		sizeof(buffer), &buffer, PSA_STORAGE_FLAG_NONE);
	UNSIGNED_LONGLONGS_EQUAL(PSA_SUCCESS, psa_status);

	/*
	 * Initializing the store should fail, because if there are two indexes with the same counter it cannot be decided
	 * which has the valid data.
	 */
	uefi_variable_store_deinit(&m_uefi_variable_store);

	efi_status = uefi_variable_store_init(&m_uefi_variable_store, OWNER_ID, MAX_VARIABLES,
					      m_persistent_backend, m_volatile_backend);
	UNSIGNED_LONGLONGS_EQUAL(EFI_LOAD_ERROR, efi_status);
}
