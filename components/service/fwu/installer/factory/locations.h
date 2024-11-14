/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INSTALLER_FACTORY_LOCATIONS_H
#define INSTALLER_FACTORY_LOCATIONS_H

/**
 * Locations represent updatable firmware subsystems. Installers target
 * locations for image installation purposes. A location is identified
 * by a UUID. Where firmware images are stored within GPT formatted
 * storage, a location UUID is used as the partition type GUID. For
 * example, the A and B partitions holding application processor
 * firmware will both be labelled with the same partition type GUID.
 * The following UUIDs identify locations used by concrete installer
 * factories as a qualifier for rules that define which installers
 * are needed to update particular locations. New location UUIDs may
 * be freely added to this file.
 */

/* Location UUID for application processor firmware */
#define LOCATION_UUID_AP_FW "2451cd6e-90fe-4b15-bf10-a69bce2d4486"

/* Location UUID for SCP firmware */
#define LOCATION_UUID_SCP_FW "691d5ea3-27fe-4104-badd-7539c00a9095"

/* Location UUID for RSE firmware */
#define LOCATION_UUID_RSE_FW "c948a156-58cb-4c38-b406-e60bff2223d5"

#endif /* INSTALLER_FACTORY_LOCATIONS_H */
