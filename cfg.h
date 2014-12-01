#pragma once

//
// Configuration storage instances
//

#include "config.h"

// We are using 2 storage instances for reliability and convenience.
// The user configuration is expected to be updated frequently by the user.
// The system configuration will never be updated during normal operation.
// Besides reliability it means that clients may obtain pointer to the
// configuration value only once during initialization.

// System config is not expected to be changed by user
extern struct config cfg_system;

// User config is expected to be changed by user
extern struct config cfg_user;

static inline void cfg_init_all()
{
	cfg_init(&cfg_system);
	cfg_init(&cfg_user);
}

