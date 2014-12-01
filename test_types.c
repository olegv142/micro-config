#include "cfg_types.h"
#include "debug.h"

const struct cfg_type g_cfg_types[] = {
	CFG_INT("test.static", 0),
	CFG_INT("test.runs", 0),
	CFG_STR_I("test.str", "0"),
	CFG_INT_I("test.arr", 0),
	// end of array marker
	CFG_TYPE_END
};

unsigned g_cfg_ntypes = sizeof(g_cfg_types)/sizeof(g_cfg_types[0]) - 1;

static inline int type_name_eq(const char* a, const char* b)
{
#define _CMP_ if (*a != *b) return 0; if (!*a) return 1; ++a; ++b;
// Compare at most 8 chars for speed
	_CMP_
	_CMP_
	_CMP_
	_CMP_
	_CMP_
	_CMP_
	_CMP_
	_CMP_
	return 1;
}

// Lookup type by name
const struct cfg_type* get_cfg_type(const char* name)
{
	const struct cfg_type* t;
	for (t = g_cfg_types; t->name; ++t) {
		if (type_name_eq(t->name, name))
			return t;
	}
	return 0;
}

// Get default value
int def_cfg_val(const struct cfg_type* t)
{
	BUG_ON(!t);
	BUG_ON(t->str);
	return t->def.val;	
}

// Get default string
const char* def_cfg_str(const struct cfg_type* t)
{
	BUG_ON(!t);
	BUG_ON(!t->str);
	return t->def.ptr;	
}

