//
// Configuration types table
//

#include "cfg_types.h"

const struct cfg_type g_cfg_types[] = {
	CFG_INT("test.static", 0),
	CFG_INT("test.runs", 0),
	CFG_STR_I("test.str", "0"),
	CFG_INT_I("test.arr", 0),
	// end of array marker
	CFG_TYPE_END
};

unsigned g_cfg_ntypes = CFG_NTYPES;
