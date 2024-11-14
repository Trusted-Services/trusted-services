/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <service/uefi/smm_variable/backend/variable_index.h>
#include <service/uefi/smm_variable/backend/variable_index_iterator.h>
#include <string>
#include <vector>

TEST_GROUP(UefiVariableIndexTests)
{
	void setup()
	{
		efi_status_t status = variable_index_init(&m_variable_index, MAX_VARIABLES);
		UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

		guid_1.Data1 = 0x12341234;
		guid_1.Data2 = 0x1234;
		guid_1.Data3 = 0x1234;
		guid_1.Data4[0] = 0x00;
		guid_1.Data4[1] = 0x01;
		guid_1.Data4[2] = 0x02;
		guid_1.Data4[3] = 0x03;
		guid_1.Data4[4] = 0x04;
		guid_1.Data4[5] = 0x05;
		guid_1.Data4[6] = 0x06;
		guid_1.Data4[7] = 0x07;

		guid_2.Data1 = 0x55443322;
		guid_2.Data2 = 0x2345;
		guid_2.Data3 = 0x2345;
		guid_2.Data4[0] = 0x10;
		guid_2.Data4[1] = 0x11;
		guid_2.Data4[2] = 0x12;
		guid_2.Data4[3] = 0x13;
		guid_2.Data4[4] = 0x14;
		guid_2.Data4[5] = 0x15;
		guid_2.Data4[6] = 0x16;
		guid_2.Data4[7] = 0x17;

		name_1 = to_variable_name(u"var1");
		name_2 = to_variable_name(u"var2_nv");
		name_3 = to_variable_name(u"var3_nv");
		null_name = to_variable_name(u"");
	}

	void teardown()
	{
		variable_index_deinit(&m_variable_index);
	}

	std::u16string to_variable_name(const char16_t *string)
	{
		std::u16string var_name(string);
		var_name.push_back(0);

		return var_name;
	}

	size_t string_get_size_in_bytes(const std::u16string &string)
	{
		return string.size() * sizeof(typename std::u16string::value_type);
	}

	void create_variables()
	{
		struct variable_info *info = NULL;

		info = variable_index_add_entry(&m_variable_index, &guid_1,
						string_get_size_in_bytes(name_1), (int16_t *) name_1.data());
		CHECK_TRUE(info);
		variable_index_set_variable(info, EFI_VARIABLE_BOOTSERVICE_ACCESS);

		info = variable_index_add_entry(&m_variable_index, &guid_2,
						string_get_size_in_bytes(name_2), (int16_t *)  name_2.data());
		CHECK_TRUE(info);
		variable_index_set_variable(info, EFI_VARIABLE_NON_VOLATILE |
							  EFI_VARIABLE_BOOTSERVICE_ACCESS);

		info = variable_index_add_entry(&m_variable_index, &guid_1,
						string_get_size_in_bytes(name_3), (int16_t *)  name_3.data());
		CHECK_TRUE(info);
		variable_index_set_variable(info, EFI_VARIABLE_NON_VOLATILE |
							  EFI_VARIABLE_RUNTIME_ACCESS |
							  EFI_VARIABLE_BOOTSERVICE_ACCESS);
	}

	static const size_t MAX_VARIABLES = 10;

	struct variable_index m_variable_index;
	EFI_GUID guid_1;
	EFI_GUID guid_2;
	std::u16string name_1;
	std::u16string name_2;
	std::u16string name_3;
	std::u16string null_name;
};

TEST(UefiVariableIndexTests, emptyIndexOperations)
{
	efi_status_t status = EFI_SUCCESS;
	struct variable_info *info = NULL;

	/* Expect not to find a variable */
	info = variable_index_find(&m_variable_index, &guid_1, string_get_size_in_bytes(name_1),
				   (const int16_t *) name_1.data());
	POINTERS_EQUAL(NULL, info);

	/* Expect also find next to be rejected */
	info = variable_index_find_next(&m_variable_index, &guid_1, string_get_size_in_bytes(name_1),
					(const int16_t *) name_1.data(), &status);
	POINTERS_EQUAL(NULL, info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_INVALID_PARAMETER, status);

	/* Remove should silently return */
	variable_index_clear_variable(&m_variable_index, info);
}

TEST(UefiVariableIndexTests, addWithOversizedName)
{
	struct variable_info *info = NULL;
	std::u16string name;

	name = to_variable_name(
		u"a long variable name that exceeds the length limit with a few chars");

	info = variable_index_add_entry(&m_variable_index, &guid_1, string_get_size_in_bytes(name),
					(int16_t *) name.data());

	/* Expect the add to fail because of an oversized name */
	POINTERS_EQUAL(NULL, info);

	name = to_variable_name(u"a long variable name that fits!");

	info = variable_index_add_entry(&m_variable_index, &guid_1, string_get_size_in_bytes(name),
					(int16_t *) name.data());

	/* Expect the add succeed */
	CHECK_TRUE(info);
}

TEST(UefiVariableIndexTests, variableIndexFull)
{
	struct variable_info *info = NULL;
	EFI_GUID guid = guid_1;

	/* Expect to be able to fill the index */
	for (size_t i = 0; i < MAX_VARIABLES; ++i) {
		info = variable_index_add_entry(&m_variable_index, &guid,
						string_get_size_in_bytes(name_1), (int16_t *) name_1.data());

		CHECK_TRUE(info);

		/* Modify the guid for the next add */
		guid.Data1 += 1;
	}

	/* Variable index should now be full */
	info = variable_index_add_entry(&m_variable_index, &guid, string_get_size_in_bytes(name_1),
					(int16_t *) name_1.data());

	POINTERS_EQUAL(NULL, info);
}

TEST(UefiVariableIndexTests, enumerateStore)
{
	const struct variable_info *info = NULL;
	efi_status_t status = EFI_NOT_FOUND;

	create_variables();

	info = variable_index_find_next(&m_variable_index, &guid_1,
					string_get_size_in_bytes(null_name), (const int16_t *) null_name.data(),
					&status);
	CHECK_TRUE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	LONGS_EQUAL(EFI_VARIABLE_BOOTSERVICE_ACCESS, info->metadata.attributes);
	MEMCMP_EQUAL(&guid_1, &info->metadata.guid, sizeof(EFI_GUID));
	MEMCMP_EQUAL(name_1.data(), info->metadata.name, name_1.size());

	info = variable_index_find_next(&m_variable_index, &info->metadata.guid,
					info->metadata.name_size, info->metadata.name, &status);
	CHECK_TRUE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	LONGS_EQUAL(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
		    info->metadata.attributes);
	MEMCMP_EQUAL(&guid_2, &info->metadata.guid, sizeof(EFI_GUID));
	MEMCMP_EQUAL(name_2.data(), info->metadata.name, name_2.size());

	info = variable_index_find_next(&m_variable_index, &info->metadata.guid,
					info->metadata.name_size, info->metadata.name, &status);
	CHECK_TRUE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	LONGS_EQUAL(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
			    EFI_VARIABLE_BOOTSERVICE_ACCESS,
		    info->metadata.attributes);
	MEMCMP_EQUAL(&guid_1, &info->metadata.guid, sizeof(EFI_GUID));
	MEMCMP_EQUAL(name_3.data(), info->metadata.name, name_3.size());

	info = variable_index_find_next(&m_variable_index, &info->metadata.guid,
					info->metadata.name_size, info->metadata.name, &status);
	POINTERS_EQUAL(NULL, info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableIndexTests, dumpLoadRoadtrip)
{
	uint8_t buffer[sizeof(uint32_t) +
		       MAX_VARIABLES * (sizeof(struct variable_metadata) + sizeof(bool))];

	create_variables();

	/* Expect the info for two NV variables to have been dumped */
	size_t dump_len = 0;
	bool is_dirty = false;
	efi_status_t status = EFI_SUCCESS;

	status = variable_index_dump(&m_variable_index, sizeof(buffer), buffer, &dump_len,
				     &is_dirty);

	CHECK_TRUE(is_dirty);
	UNSIGNED_LONGS_EQUAL(EFI_SUCCESS, status);
	/*
	 * Variable index counter is at the beginning, which is followed by metadata and
	 * constraint status byte of both NV variables
	 */
	UNSIGNED_LONGS_EQUAL(sizeof(uint32_t) +
				     ((sizeof(struct variable_metadata) + sizeof(bool)) * 2),
			     dump_len);

	/* Expect no records to be dirty when the dump is repeated */
	dump_len = 0;
	status = variable_index_dump(&m_variable_index, sizeof(buffer), buffer, &dump_len,
				     &is_dirty);

	UNSIGNED_LONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_FALSE(is_dirty);
	UNSIGNED_LONGS_EQUAL(sizeof(uint32_t) +
				     ((sizeof(struct variable_metadata) + sizeof(bool)) * 2),
			     dump_len);

	/* Tear down and reinitialize to simulate a reboot */
	variable_index_deinit(&m_variable_index);
	status = variable_index_init(&m_variable_index, MAX_VARIABLES);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);

	/* Load the dumped contents */
	size_t load_len = variable_index_restore(&m_variable_index, dump_len, buffer);
	UNSIGNED_LONGS_EQUAL(dump_len, load_len);

	/* Enumerate and now expect only NV variables to be present */
	status = EFI_NOT_FOUND;
	const struct variable_info *info = NULL;

	info = variable_index_find_next(&m_variable_index, &guid_1,
					string_get_size_in_bytes(null_name),  (const int16_t *) null_name.data(),
					&status);
	CHECK_TRUE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGLONGS_EQUAL(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				 info->metadata.attributes);

	info = variable_index_find_next(&m_variable_index, &info->metadata.guid,
					info->metadata.name_size, info->metadata.name, &status);
	CHECK_TRUE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGLONGS_EQUAL(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
					 EFI_VARIABLE_BOOTSERVICE_ACCESS,
				 info->metadata.attributes);

	info = variable_index_find_next(&m_variable_index, &info->metadata.guid,
					info->metadata.name_size, info->metadata.name, &status);
	POINTERS_EQUAL(NULL, info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableIndexTests, dumpLoadConstrainedVariable)
{
	uint8_t buffer[sizeof(uint32_t) +
		       MAX_VARIABLES * (sizeof(struct variable_metadata) + sizeof(bool))];

	create_variables();

	struct variable_constraints constraints;
	constraints.revision = 10;
	constraints.property = VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
	constraints.attributes = 0;
	constraints.min_size = 1;
	constraints.max_size = 100;

	/* Set check constraints on one of the variables */
	struct variable_info *info = variable_index_find(&m_variable_index, &guid_2,
							 string_get_size_in_bytes(name_2),
							 (const int16_t *)name_2.data());

	CHECK_TRUE(info);
	CHECK_TRUE(info->is_variable_set);
	CHECK_FALSE(info->is_constraints_set);

	variable_index_set_constraints(info, &constraints);

	CHECK_TRUE(info->is_constraints_set);
	CHECK_TRUE(info->is_variable_set);

	size_t dump_len = 0;
	bool is_dirty = false;
	efi_status_t status = EFI_SUCCESS;
	status = variable_index_dump(&m_variable_index, sizeof(buffer), buffer, &dump_len,
				     &is_dirty);

	UNSIGNED_LONGS_EQUAL(EFI_SUCCESS, status);
	CHECK_TRUE(is_dirty);

	/*
	 * Variable index counter is at the beginning, which is followed by metadata and
	 * constraint status byte of both NV variables, but only one of them has
	 * constraints
	 */
	UNSIGNED_LONGS_EQUAL(sizeof(uint32_t) +
				     (sizeof(struct variable_metadata) + sizeof(bool)) * 2 +
				     sizeof(struct variable_constraints),
			     dump_len);

	/* Load the dumped contents */
	size_t load_len = variable_index_restore(&m_variable_index, dump_len, buffer);
	UNSIGNED_LONGS_EQUAL(dump_len, load_len);
}

TEST(UefiVariableIndexTests, dumpBufferTooSmall)
{
	/*
	 * Enough to fit the variable index counter and the metadata and constraint
	 * status of a single variable
	 */
	uint8_t buffer[sizeof(uint32_t) + sizeof(struct variable_metadata) + sizeof(bool)];

	create_variables();

	/* There should be two NV variables whose info needs saving. The buffer provided
	 * however is only big enough for one. Expect the dumped data length to not
	 * exceed the length of the buffer.
	 */
	size_t dump_len = 0;
	bool is_dirty = false;
	efi_status_t status = EFI_SUCCESS;

	status = variable_index_dump(&m_variable_index, sizeof(buffer), buffer, &dump_len,
				     &is_dirty);

	CHECK_TRUE(is_dirty);
	UNSIGNED_LONGS_EQUAL(EFI_BUFFER_TOO_SMALL, status);
	UNSIGNED_LONGS_EQUAL(0, dump_len);
}

TEST(UefiVariableIndexTests, removeVariable)
{
	uint8_t buffer[sizeof(uint32_t) +
		       MAX_VARIABLES * (sizeof(struct variable_metadata) + sizeof(bool))];
	struct variable_info *info = NULL;

	create_variables();

	/* Remove one of the NV variables */
	info = variable_index_find(&m_variable_index, &guid_2, string_get_size_in_bytes(name_2),
				   (const int16_t *) name_2.data());

	variable_index_clear_variable(&m_variable_index, info);

	/* Expect index to be dirty and for only one NV variable to be left */
	size_t dump_len = 0;
	bool is_dirty = false;
	efi_status_t status = EFI_SUCCESS;
	status = variable_index_dump(&m_variable_index, sizeof(buffer), buffer, &dump_len,
				     &is_dirty);

	CHECK_TRUE(is_dirty);
	UNSIGNED_LONGS_EQUAL(EFI_SUCCESS, status);
	/*
	 * Dump to now contains the variable index counter and metadata,
	 * constraint status data of a variable
	 */
	UNSIGNED_LONGS_EQUAL(sizeof(uint32_t) + sizeof(struct variable_metadata) + sizeof(bool),
			     dump_len);

	/* Remove the volatile variable */
	info = variable_index_find(&m_variable_index, &guid_1, string_get_size_in_bytes(name_1),
				   (const int16_t *) name_1.data());

	variable_index_clear_variable(&m_variable_index, info);

	/* Expect index not to be dirty because there was no change to any NV variable */
	dump_len = 0;
	status = variable_index_dump(&m_variable_index, sizeof(buffer), buffer, &dump_len,
				     &is_dirty);

	CHECK_FALSE(is_dirty);
	UNSIGNED_LONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(uint32_t) + sizeof(struct variable_metadata) + sizeof(bool),
			     dump_len);

	/* Remove the remaining NV variable */
	info = variable_index_find(&m_variable_index, &guid_1, string_get_size_in_bytes(name_3),
				   (const int16_t *) name_3.data());

	variable_index_clear_variable(&m_variable_index, info);

	/* Expect index to be dirty and dump to now contains only the variable index counter */
	dump_len = 0;
	status = variable_index_dump(&m_variable_index, sizeof(buffer), buffer, &dump_len,
				     &is_dirty);

	CHECK_TRUE(is_dirty);
	UNSIGNED_LONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(uint32_t), dump_len);

	/* Enumerate and now expect an empty index */
	info = NULL;

	info = variable_index_find_next(&m_variable_index, &guid_1,
					string_get_size_in_bytes(null_name),  (const int16_t *) null_name.data(),
					&status);
	POINTERS_EQUAL(NULL, info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);
}

TEST(UefiVariableIndexTests, checkIterator)
{
	struct variable_info *info = NULL;

	create_variables();

	struct variable_index_iterator iter;

	variable_index_iterator_first(&iter, &m_variable_index);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check first entry is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_1), info->metadata.name_size);
	MEMCMP_EQUAL(name_1.data(), info->metadata.name, info->metadata.name_size);

	variable_index_iterator_next(&iter);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check next is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_2), info->metadata.name_size);
	MEMCMP_EQUAL(name_2.data(), info->metadata.name, info->metadata.name_size);

	struct variable_info *info_to_remove = info;

	variable_index_iterator_next(&iter);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check next is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_3), info->metadata.name_size);
	MEMCMP_EQUAL(name_3.data(), info->metadata.name, info->metadata.name_size);

	/* Expect iterating to be done */
	variable_index_iterator_next(&iter);
	CHECK_TRUE(variable_index_iterator_is_done(&iter));

	/* Now remove the middle entry */
	variable_index_clear_variable(&m_variable_index, info_to_remove);
	variable_index_remove_unused_entry(&m_variable_index, info_to_remove);

	/* Iterate again but this time there should only be two entries */
	variable_index_iterator_first(&iter, &m_variable_index);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check first entry is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_1), info->metadata.name_size);
	MEMCMP_EQUAL(name_1.data(), info->metadata.name, info->metadata.name_size);

	variable_index_iterator_next(&iter);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check next entry is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_3), info->metadata.name_size);
	MEMCMP_EQUAL(name_3.data(), info->metadata.name, info->metadata.name_size);

	/* Expect iterating to be done */
	variable_index_iterator_next(&iter);
	CHECK_TRUE(variable_index_iterator_is_done(&iter));
}

TEST(UefiVariableIndexTests, setCheckConstraintsExistingVar)
{
	/* Variable check constraints are set using an independent SMM
	 * function from setting a variable. A client may set constraints
	 * for an existing variable or a non-existing one. This test case
	 * covers setting check constraints for an existing variable.
	 */
	create_variables();

	struct variable_constraints constraints;
	constraints.revision = 10;
	constraints.property = VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
	constraints.attributes = 0;
	constraints.min_size = 1;
	constraints.max_size = 100;

	/* Set check constraints on one of the variables */
	struct variable_info *info = variable_index_find(
		&m_variable_index, &guid_2, string_get_size_in_bytes(name_2), (const int16_t *) name_2.data());

	CHECK_TRUE(info);
	CHECK_TRUE(info->is_variable_set);
	CHECK_FALSE(info->is_constraints_set);

	variable_index_set_constraints(info, &constraints);

	CHECK_TRUE(info->is_constraints_set);
	CHECK_TRUE(info->is_variable_set);

	/* Remove the variable but still expect the variable to be indexed
	 * because of the set constraints.
	 */
	variable_index_clear_variable(&m_variable_index, info);

	info = variable_index_find(&m_variable_index, &guid_2, string_get_size_in_bytes(name_2),
				   (const int16_t *) name_2.data());

	CHECK_TRUE(info);
	CHECK_FALSE(info->is_variable_set);
	CHECK_TRUE(info->is_constraints_set);

	/* Enumerate over variables, only expecting to find the two remaining 'set' variables. */
	info = NULL;
	efi_status_t status = EFI_NOT_FOUND;

	info = variable_index_find_next(&m_variable_index, &guid_1,
					string_get_size_in_bytes(null_name),  (const int16_t *) null_name.data(),
					&status);
	CHECK_TRUE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGLONGS_EQUAL(EFI_VARIABLE_BOOTSERVICE_ACCESS, info->metadata.attributes);

	info = variable_index_find_next(&m_variable_index, &info->metadata.guid,
					info->metadata.name_size, info->metadata.name, &status);
	CHECK_TRUE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_SUCCESS, status);
	UNSIGNED_LONGLONGS_EQUAL(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
					 EFI_VARIABLE_BOOTSERVICE_ACCESS,
				 info->metadata.attributes);

	info = variable_index_find_next(&m_variable_index, &info->metadata.guid,
					info->metadata.name_size, info->metadata.name, &status);
	CHECK_FALSE(info);
	UNSIGNED_LONGLONGS_EQUAL(EFI_NOT_FOUND, status);

	/* Iterating over the index should still return all three because the set constraints
	 * for variable 2 still persist.
	 */
	struct variable_index_iterator iter;

	variable_index_iterator_first(&iter, &m_variable_index);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check first entry is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_1), info->metadata.name_size);
	MEMCMP_EQUAL(name_1.data(), info->metadata.name, info->metadata.name_size);

	variable_index_iterator_next(&iter);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check next is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_2), info->metadata.name_size);
	MEMCMP_EQUAL(name_2.data(), info->metadata.name, info->metadata.name_size);

	variable_index_iterator_next(&iter);
	CHECK_FALSE(variable_index_iterator_is_done(&iter));

	/* Check next is as expected */
	info = variable_index_iterator_current(&iter);
	CHECK_TRUE(info);
	UNSIGNED_LONGS_EQUAL(string_get_size_in_bytes(name_3), info->metadata.name_size);
	MEMCMP_EQUAL(name_3.data(), info->metadata.name, info->metadata.name_size);

	/* Expect iterating to be done */
	variable_index_iterator_next(&iter);
	CHECK_TRUE(variable_index_iterator_is_done(&iter));
}

TEST(UefiVariableIndexTests, setCheckConstraintsNonExistingVar)
{
	/* This test case covers setting check constraints for a variable
	 * that hasn't been set yet.
	 */
	struct variable_constraints constraints;
	constraints.revision = 10;
	constraints.property = VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY;
	constraints.attributes = 0;
	constraints.min_size = 1;
	constraints.max_size = 100;

	/* Initially expect no variable_info */
	struct variable_info *info = variable_index_find(
		&m_variable_index, &guid_2, string_get_size_in_bytes(name_2),
		(const int16_t *) name_2.data());

	CHECK_FALSE(info);

	/* Adding the check constraints should result in an entry being added */
	info = variable_index_add_entry(
		&m_variable_index, &guid_2, string_get_size_in_bytes(name_2),
		(const int16_t *) name_2.data());
	CHECK_TRUE(info);

	variable_index_set_constraints(info, &constraints);
	CHECK_FALSE(info->is_variable_set);
	CHECK_TRUE(info->is_constraints_set);

	/* Updating the variable should cause the variable to be marked as set */
	variable_index_set_variable(info,
				    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS);

	CHECK_TRUE(info->is_variable_set);
	CHECK_TRUE(info->is_constraints_set);

	/* Check the constraints are as expected */
	UNSIGNED_LONGS_EQUAL(constraints.revision, info->check_constraints.revision);
	UNSIGNED_LONGS_EQUAL(constraints.property, info->check_constraints.property);
	UNSIGNED_LONGS_EQUAL(constraints.attributes, info->check_constraints.attributes);
	UNSIGNED_LONGS_EQUAL(constraints.min_size, info->check_constraints.min_size);
	UNSIGNED_LONGS_EQUAL(constraints.max_size, info->check_constraints.max_size);
}
