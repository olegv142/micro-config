#pragma once

//
// Configuration types registry
//

// The configuration types are derived from either int or string
// base types. They may be either singletons or have 1D or 2D
// array of instances. Every configuration type has distinct name
// and default value common for all instances.

struct cfg_type {
	const char* name;
	unsigned char nind; // 0..2 for int, 0..1 for str
	unsigned char str;  // is string flag
	union { // default value
		int val;
		const char* ptr;
	} def;
};

#define CFG_INT(n, d)    {.name=n, .nind=0, .str=0, .def.val=d}
#define CFG_INT_I(n, d)  {.name=n, .nind=1, .str=0, .def.val=d}
#define CFG_INT_IJ(n, d) {.name=n, .nind=2, .str=0, .def.val=d}
#define CFG_STR(n, d)    {.name=n, .nind=0, .str=1, .def.ptr=d}
#define CFG_STR_I(n, d)  {.name=n, .nind=1, .str=1, .def.ptr=d}
#define CFG_TYPE_END     {.name=0}

#define MAX_CFG_TYPES 255

// Global types registry
extern const struct cfg_type g_cfg_types[];
extern unsigned g_cfg_ntypes; // number of types

// Lookup type by name
const struct cfg_type* get_cfg_type(const char* name);

// Get default value
int def_cfg_val(const struct cfg_type* t);

// Get default string
const char* def_cfg_str(const struct cfg_type* t);
