#pragma once

#include "cfg_types.h"
#include "flash.h"
#include "crc16.h"

//
// Configuration storage
//

// It uses flash for storing configuration types instances.
// Every storage instance needs 2 flash buffers (areas)
// since when we are erasing one the another one keeps
// configuration content.

#define CFG_BUFF_SEGS 2
#define CFG_BUFF_SIZE (FLASH_SEG_SZ*CFG_BUFF_SEGS)

typedef enum {
	area_empty,
	area_dirty,
	area_open,
	area_modified,
	area_closed,
	area_completed
} cfg_area_status_t;

extern const char* cfg_area_status_names[];

// Storage buffer
struct config_area {
	const char*       storage;
	unsigned          used_bytes;
	crc16_t           crc;
	unsigned char     status;
	unsigned char     invalid;	
};

// The config instance has 2 buffer areas and keeps index of the active one
struct config {
	struct config_area area[2];
	int                active_area;
};

// The header is written before actual data (config type instance)
struct cfg_obj_header {
	unsigned char type_tag; // index of the type in global registry
	union {
		struct {
			unsigned char i:4;    // first index
			unsigned char j_sz:4; // second index or string size not including 0-termination
		};
		unsigned char ij;
	};
};

// The footer is written right after the data
struct cfg_obj_footer {
	crc16_t crc;
};

// Initialize the particular config instance
void cfg_init(struct config* cfg);

typedef void (*cfg_enum_cb)(struct config*, struct cfg_type const*, struct cfg_obj_header const*, struct cfg_obj_footer const*);

// Enumerate objects
void cfg_enum(struct config* cfg, cfg_enum_cb cb);

// Lookup value in storage and return default if not found
int cfg_get_val_(struct config* cfg, const struct cfg_type* t, unsigned char i, unsigned char j);
const char* cfg_get_str_(struct config* cfg, const struct cfg_type* t, unsigned char i);

//
// The shorthand variants of lookup routine
//

static inline int cfg_get_val(struct config* cfg, const struct cfg_type* t)
{
	return cfg_get_val_(cfg, t, 0, 0);
}

static inline int cfg_get_val_i(struct config* cfg, const struct cfg_type* t, unsigned char i)
{
	return cfg_get_val_(cfg, t, i, 0);
}

static inline int cfg_get_val_ij(struct config* cfg, const struct cfg_type* t, unsigned char i, unsigned char j)
{
	return cfg_get_val_(cfg, t, i, j);
}

static inline const char* cfg_get_str(struct config* cfg, const struct cfg_type* t)
{
	return cfg_get_str_(cfg, t, 0);
}

static inline const char* cfg_get_str_i(struct config* cfg, const struct cfg_type* t, unsigned char i)
{
	return cfg_get_str_(cfg, t, i);
}

// Put value to the storage. Returns 0 on success, -1 if no space left on the storage.
int cfg_put_val_(struct config* cfg, const struct cfg_type* t, unsigned char i, unsigned char j, int val);

#define MAX_CFG_STR_LEN 15

// Put zero-terminated string to the storage. If string length exceeds MAX_CFG_STR_LEN the -1 will be returned.
int cfg_put_str_(struct config* cfg, const struct cfg_type* t, unsigned char i, const char* str);

//
// The shorthand variants of put routine
//

static inline int cfg_put_val(struct config* cfg, const struct cfg_type* t, int val)
{
	return cfg_put_val_(cfg, t, 0, 0, val);
}

static inline int cfg_put_val_i(struct config* cfg, const struct cfg_type* t, unsigned char i, int val)
{
	return cfg_put_val_(cfg, t, i, 0, val);
}

static inline int cfg_put_val_ij(struct config* cfg, const struct cfg_type* t, unsigned char i, unsigned char j, int val)
{
	return cfg_put_val_(cfg, t, i, j, val);
}

static inline int cfg_put_str(struct config* cfg, const struct cfg_type* t, const char* str)
{
	return cfg_put_str_(cfg, t, 0, str);
}

static inline int cfg_put_str_i(struct config* cfg, const struct cfg_type* t, unsigned char i, const char* str)
{
	return cfg_put_str_(cfg, t, i, str);
}
