#include "cfg.h"

#pragma data_alignment=FLASH_SEG_SZ
static __no_init const char cfg_system_storage[2][CFG_BUFF_SIZE] @ "CODE";

#pragma data_alignment=FLASH_SEG_SZ
static __no_init const char cfg_user_storage[2][CFG_BUFF_SIZE] @ "CODE";

// System config is not expected to be changed by user
struct config cfg_system = {{
		{cfg_system_storage[0]},
		{cfg_system_storage[1]}
	},
	.active_area = -1
};

// User config is expected to be changed by user
struct config cfg_user = {{
		{cfg_user_storage[0]},
		{cfg_user_storage[1]}
	},
	.active_area = -1
};
