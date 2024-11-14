/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_ramstore.h"
#include "config/interface/config_store.h"
#include "trace.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * Variable length container for a configuration object.
 */
struct config_container
{
	enum config_classifier classifier;
	char name[32];
	unsigned int instance;
	size_t size;
	struct config_container *next;
};

static struct config_container *config_container_create(enum config_classifier classifier,
	const char *name, unsigned int instance,
	const void *data, size_t size)
{
	struct config_container *container = NULL;
	size_t name_length = strlen(name) + 1;

	if (name_length > sizeof(container->name))
		return NULL;

	container = malloc(sizeof(struct config_container) + size);
	if (!container)
		return NULL;

	container->classifier = classifier;
	memcpy(container->name, name, name_length);
	container->instance = instance;
	container->size = size;
	container->next = NULL;

	memcpy((uint8_t *)container + sizeof(struct config_container), data, size);

	return container;
}

static void config_container_destroy(struct config_container *container)
{
	free(container);
}

static const void *config_container_data(const struct config_container *container)
{
	return (const uint8_t*)container + sizeof(struct config_container);
}

/**
 * Singleton config_ramstore instance
 */
static struct config_ramstore
{
	struct config_container *object_list;
} ramstore = {0};


void config_ramstore_init(void)
{

}

void config_ramstore_deinit(void)
{
	while (ramstore.object_list) {

		struct config_container *next = ramstore.object_list->next;
		config_container_destroy(ramstore.object_list);
		ramstore.object_list = next;
	}
}

bool config_store_add(enum config_classifier classifier,
	const char *name,
	unsigned int instance,
	const void *data,
	size_t data_len)
{
	struct config_container *container;

	container = config_container_create(classifier, name, instance, data, data_len);
	if (!container) return false;

	container->next = ramstore.object_list;
	ramstore.object_list = container;

	return true;
}

bool config_store_query(enum config_classifier classifier,
	const char *name,
	unsigned int instance,
	void *data,
	size_t data_buf_size)
{
	bool success = false;
	const struct config_container *container = ramstore.object_list;

	while (container) {

		if ((container->classifier == classifier) &&
			(strncmp(container->name, name, sizeof(container->name)) == 0) &&
			(container->instance == instance)) {

			if (data_buf_size == container->size) {
				memcpy(data, config_container_data(container), container->size);
				success = true;
			} else {
				DMSG("Query with different size (%lu != %lu)", data_buf_size,
				     container->size);
			}

			break;
		}

		container = container->next;
	}

	if (!success)
		DMSG("Failed to query data with name %s", name);

	return success;
}

unsigned int config_store_count(enum config_classifier classifier)
{
	unsigned int count = 0;

	const struct config_container *container = ramstore.object_list;

	while (container) {

		if (container->classifier == classifier) ++count;
		container = container->next;
	}

	return count;
}
