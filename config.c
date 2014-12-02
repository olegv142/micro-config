#include "config.h"
#include "debug.h"
#include "string.h"
#include "uart.h"

const char* cfg_area_status_names[] = {
	"empty",
	"dirty",
	"open",
	"modified",
	"closed",
	"completed"
};

static inline unsigned char cfg_type2tag(struct cfg_type const* t)
{
	return t ? t - g_cfg_types : ~0;
}

static struct cfg_type const* cfg_get_obj_type(struct cfg_obj_header const* h)
{
	if (h->type_tag > g_cfg_ntypes)
		return 0;
	return &g_cfg_types[h->type_tag];
}

static inline unsigned cfg_obj_data_size_unaligned_(struct cfg_type const* t, unsigned char str_sz)
{
	return t->str ? str_sz + 1 : sizeof(int);
}

static inline unsigned cfg_obj_data_size_(struct cfg_type const* t, unsigned char str_sz)
{
	if (!t) {
		return 0;
	} else if (t->str) {
		unsigned sz = str_sz + 1;
		if (sz & 1) sz += 1;
		return sz;
	} else
		return sizeof(int);
}

static inline unsigned cfg_obj_data_size(struct cfg_type const* t, struct cfg_obj_header const* h)
{
	return cfg_obj_data_size_(t, h->j_sz);
}

// Returns the size of the object representation including header and footer
static inline unsigned cfg_obj_size_(struct cfg_type const* t, struct cfg_obj_header const* h)
{
	return sizeof(struct cfg_obj_header) + cfg_obj_data_size(t, h) + sizeof(struct cfg_obj_footer);
}

static inline unsigned cfg_obj_size(struct cfg_obj_header const* h)
{
	return cfg_obj_size_(cfg_get_obj_type(h), h);
}

static inline void cfg_init_hdr(struct cfg_type const* t, struct cfg_obj_header* h, int i, int j)
{
	h->type_tag = cfg_type2tag(t);
	h->j_sz = j;
	h->i = i;
}

static inline int cfg_is_status_tag(struct cfg_obj_header const* h)
{
	return !(unsigned char)~h->type_tag;
}

// Check if 2 objects represents different versions of the same object instance
static inline int cfg_same_instance(struct cfg_type const* t, struct cfg_obj_header const* h, struct cfg_obj_header const* h_)
{
	if (h->type_tag != h_->type_tag)
		return 0;
	return t->str ? h->i == h_->i : h->ij == h_->ij;
}

static inline struct config_area* cfg_active_area(struct config* cfg)
{
	int a = cfg->active_area;
	BUG_ON(a < 0);
	return &cfg->area[a];
}

static void cfg_erase_area(struct config* cfg, signed char a)
{
	struct config_area* ar = &cfg->area[a];
	flash_erase(ar->storage, CFG_BUFF_SEGS);
	ar->used_bytes = 0;
	ar->crc = CRC16_INIT;
	ar->status = area_empty;
	ar->invalid = 0;
}

static inline void cfg_erase_all(struct config* cfg)
{
	cfg_erase_area(cfg, 0);
	cfg_erase_area(cfg, 1);
	cfg->active_area = 0;
}

static int cfg_is_area_clean(struct config* cfg, signed char a)
{
	struct config_area* ar = &cfg->area[a];
	unsigned const* ptr = (unsigned const*)(ar->storage + ar->used_bytes);
	unsigned const* end = (unsigned const*)(ar->storage + CFG_BUFF_SIZE);
	for (; ptr < end; ++ptr) {
		if (~*ptr)
			return 0;
	}
	return 1;
}

static void cfg_chk_area(struct config* cfg, signed char a)
{
	struct config_area* ar = &cfg->area[a];
	char const *ptr, *ptr_, *end;
	ar->crc = CRC16_INIT;
	ar->status = area_empty;
	for (ptr = ar->storage, end = ptr + CFG_BUFF_SIZE; ptr < end; ptr = ptr_)
	{
		unsigned sz;
		struct cfg_obj_footer const* f;
		struct cfg_obj_header const* h = (struct cfg_obj_header const*)ptr;
		struct cfg_type const* t = cfg_get_obj_type(h);
		if (!t) {
			if ((unsigned char)~h->type_tag)
				goto invalid;
			if (!(unsigned char)~h->ij) // header is all ones
				break;
		}
		sz = cfg_obj_size_(t, h);
		ptr_ = ptr + sz;
		if (ptr_ > end)
			goto invalid;
		f = (struct cfg_obj_footer const*)ptr_ - 1;
		ar->crc = crc16_up_buff(ar->crc, ptr, sz - sizeof(*f));
		if (f->crc != ar->crc)
			goto invalid;
		if (!t) {
			switch (h->ij) {
			case area_open:
				if (ar->status != area_empty && ar->status != area_dirty)
					goto invalid_status;
				ar->status = area_open;
				break;
			case area_closed:
				if (ar->status != area_open && ar->status != area_modified)
					goto invalid_status;
				ar->status = area_closed;
				break;
			case area_completed:
				if (ar->status != area_closed && ar->status != area_empty)
					goto invalid_status;
				ar->status = area_completed;
				break;
			default:
				goto invalid_status;
			}
		} else {
			if (ar->status > area_modified)
				goto invalid_status;
			switch (ar->status) {
			case area_empty:
				ar->status = area_dirty;
				break;
			case area_open:
				ar->status = area_modified;
				break;
			}
		}
		continue;
invalid_status:
		DBG_BUG();
invalid:
		ar->invalid = 1;
		break;
	}
	ar->used_bytes = ptr - ar->storage;
	BUG_ON(ar->used_bytes > CFG_BUFF_SIZE);
	if (!ar->invalid && !cfg_is_area_clean(cfg, a))
		ar->invalid = 1;
}

static int cfg_has_instance(struct config* cfg, struct cfg_obj_header const* h, unsigned range)
{
	char const *ptr = cfg_active_area(cfg)->storage, *end = ptr + range;
	struct cfg_type const* t = cfg_get_obj_type(h);
	while (ptr < end) {
		struct cfg_obj_header const* h_ = (struct cfg_obj_header const*)ptr;
		if (cfg_same_instance(t, h, h_))
			return 1;
		ptr += cfg_obj_size(h_);
	}
	return 0;
}

// Enumerate objects
void cfg_enum(struct config* cfg, cfg_enum_cb cb)
{
	struct config_area const* ar = cfg_active_area(cfg);
	unsigned used_bytes = ar->used_bytes;
	if (!used_bytes)
		return;
	// We are going to find all latest versions of all objects
	// and do it exactly once
	char const *ptr = ar->storage, *end = ptr + used_bytes, *next_ptr = 0, *ptr_;
	for (; ptr; ptr = next_ptr)
	{
		unsigned processed_range = ptr - ar->storage;
		struct cfg_obj_header const* h = (struct cfg_obj_header const*)ptr;
		struct cfg_type const* t = cfg_get_obj_type(h);
		ptr += cfg_obj_size_(t, h);
		if (!t) { // skip status tags
			next_ptr = ptr < end ? ptr : 0;
			continue;
		}
		struct cfg_obj_footer const* f = (struct cfg_obj_footer const*)ptr - 1;
		// Find latest version of the selected instance and find
		// the next unprocessed instance
		for (next_ptr = 0; ptr < end; ptr = ptr_)
		{
			struct cfg_obj_header const* h_ = (struct cfg_obj_header const*)ptr;
			ptr_ = ptr + cfg_obj_size(h_);
			BUG_ON(ptr_ > end);
			if (cfg_is_status_tag(h_)) {
				continue;
			}
			if (cfg_same_instance(t, h, h_)) {
				// more recent version
				struct cfg_obj_footer const* f_ = (struct cfg_obj_footer const*)ptr_ - 1;
				h = h_;
				f = f_;
				continue;
			}
			if (!next_ptr && !cfg_has_instance(cfg, h_, processed_range)) {
				// next unprocessed instance
				next_ptr = ptr;
			}
		}
		cb(cfg, t, h, f);
	}
}

// The space is reserved for closed + completed records
#define CFG_RESERVED_SPACE (2*(sizeof(struct cfg_obj_header)+sizeof(struct cfg_obj_footer)))

// Append value to the current area. Returns written object size on success, 0 if no space left.
static unsigned cfg_write(
		struct config* cfg, int a,
		const struct cfg_type* t,
		void const* pval,
		unsigned char i,
		unsigned char j
	)
{
	unsigned val_sz  = cfg_obj_data_size_(t, j);
	unsigned sz = sizeof(struct cfg_obj_header) + val_sz + sizeof(struct cfg_obj_footer);
	struct config_area* ar = &cfg->area[a];

	BUG_ON(ar->invalid);
	BUG_ON(ar->used_bytes > CFG_BUFF_SIZE);
	if (ar->used_bytes + sz > (t ? CFG_BUFF_SIZE-CFG_RESERVED_SPACE : CFG_BUFF_SIZE))
		return 0;

	const char* ptr = ar->storage + ar->used_bytes;
	struct cfg_obj_footer f;
	struct cfg_obj_header h;
	cfg_init_hdr(t, &h, i, j);
	flash_write((unsigned*)ptr, (unsigned const*)&h, sizeof(h));
	ar->crc = crc16_up_buff(ar->crc, &h, sizeof(h));
	ptr += sizeof(h);
	if (val_sz) {
		unsigned val_sz_ = cfg_obj_data_size_unaligned_(t, j);
		flash_write((unsigned*)ptr, (unsigned const*)pval, val_sz_);
		ar->crc = crc16_up_buff(ar->crc, pval, val_sz_);
		ptr += val_sz;
		if (val_sz_ & 1)
			ar->crc = crc16_up(ar->crc, 0xff);
		ar->status = ar->status < area_open ? area_dirty : area_modified;
	}
	f.crc = ar->crc;
	flash_write((unsigned*)ptr, (unsigned const*)&f, sizeof(f));	
	ar->used_bytes += sz;
	return sz;
}

static void cfg_commit_status(struct config* cfg, int a, cfg_area_status_t st)
{
	unsigned res;
	if (cfg->area[a].invalid || cfg->area[a].status >= st)
		return;
	res = cfg_write(cfg, a, 0, 0, st, 0);
	BUG_ON(!res);
	cfg->area[a].status = st;
}

void cfg_chkpt_cb(struct config* cfg, struct cfg_type const* t, struct cfg_obj_header const* h, struct cfg_obj_footer const* f)
{
	// write it to snapshot
	int a = cfg->active_area;
	unsigned res = cfg_write(cfg, a ^ 1, t, h + 1, h->i, h->j_sz);
	BUG_ON(!res);
}

// Checkpoint storage.
// Make the snapshot of the active area onto the inactive one and make them active
static void cfg_chkpt(struct config* cfg)
{
	BUG_ON(cfg->active_area < 0);
	int old_a = cfg->active_area, new_a = old_a ^ 1;
	BUG_ON(cfg->area[old_a].status < area_open);
	cfg_commit_status(cfg, old_a, area_closed);
	cfg_erase_area(cfg, new_a);
	cfg_enum(cfg, cfg_chkpt_cb);
	cfg_commit_status(cfg, new_a, area_open);
	cfg_commit_status(cfg, old_a, area_completed);
	cfg->active_area = new_a;
}

static unsigned cfg_put(
		struct config* cfg,
		const struct cfg_type* t,
		void const* pval,
		unsigned char i,
		unsigned char j_sz
	)
{
	unsigned res;
	int a = cfg->active_area;
	BUG_ON(a < 0);
	res = cfg_write(cfg, cfg->active_area, t, pval, i, j_sz);
	if (res)
		return res;
	// Free space by check pointing
	cfg_chkpt(cfg);
	return cfg_write(cfg, cfg->active_area, t, pval, i, j_sz);
}

static inline int cfg_select_active_area(struct config* cfg)
{
	// area has no usable data
	if (cfg->area[1].status < area_open)
		return 0;
	if (cfg->area[0].status < area_open)
		return 1;
	// area successfully open
	if (cfg->area[0].status < area_closed)
		return 0;
	if (cfg->area[1].status < area_closed)
		return 1;
	// in the middle of checkpoint
	if (cfg->area[0].status == area_closed)
		return 0;
	if (cfg->area[1].status == area_closed)
		return 1;
	// checkpoint completed
	if (cfg->area[1].status == area_completed)
		return 0;
	if (cfg->area[0].status == area_completed)
		return 1;
	BUG(); // Should never hit
	return -1;
}

void cfg_init(struct config* cfg)
{
	cfg_chk_area(cfg, 0);
	cfg_chk_area(cfg, 1);
	// Choose active area
	cfg->active_area = cfg_select_active_area(cfg);
	if (cfg->active_area < 0 || cfg->area[cfg->active_area].status < area_open) {
		// Prepare active area if necessary
		cfg_erase_all(cfg);
		cfg_commit_status(cfg, cfg->active_area,   area_open);
		cfg_commit_status(cfg, cfg->active_area^1, area_completed);
	}
	else if (
		cfg->area[cfg->active_area].status != area_open ||
		cfg->area[cfg->active_area^1].status != area_completed ||
		cfg->area[cfg->active_area].invalid
		
	) {
		// Stabilize storage
		cfg_chkpt(cfg);
	}
	// Erase failed inactive area
	if (cfg->area[cfg->active_area^1].invalid)
		cfg_erase_area(cfg, cfg->active_area^1);
	// We should never start from invalid active area
	BUG_ON(cfg->area[cfg->active_area].invalid);
}

// Lookup value in storage
static void const* cfg_lookup(struct config* cfg, const struct cfg_type* t, unsigned char i, unsigned char j)
{
	void const* obj = 0;
	struct config_area* ar = cfg_active_area(cfg);
	char const *ptr = ar->storage, *end = ptr + ar->used_bytes;
	struct cfg_obj_header h;
	BUG_ON(j && t->str);
	cfg_init_hdr(t, &h, i, j);
	while (ptr < end) {
		struct cfg_obj_header const* h_ = (struct cfg_obj_header const*)ptr;
		if (cfg_same_instance(t, &h, h_)) {
			obj = h_ + 1;
		}
		ptr += cfg_obj_size(h_);
	}
	return obj;
}

// Lookup value in storage and return default if not found
int cfg_get_val_(struct config* cfg, const struct cfg_type* t, unsigned char i, unsigned char j)
{
	BUG_ON(t->str);
	BUG_ON(i && t->nind < 1);
	BUG_ON(j && t->nind < 2);
	void const* ptr = cfg_lookup(cfg, t, i, j);
	if (ptr)
		return *(int const*)ptr;
	return def_cfg_val(t);
}

const char* cfg_get_str_(struct config* cfg, const struct cfg_type* t, unsigned char i)
{
	BUG_ON(!t->str);
	BUG_ON(i && t->nind < 1);
	void const* ptr = cfg_lookup(cfg, t, i, 0);
	if (ptr)
		return (char const*)ptr;
	return def_cfg_str(t);
}

// Put value to the storage. Returns 0 on success, -1 if no space left on the storage.
int cfg_put_val_(struct config* cfg, const struct cfg_type* t, unsigned char i, unsigned char j, int val)
{
	BUG_ON(t->str);
	BUG_ON(i && t->nind < 1);
	BUG_ON(j && t->nind < 2);
	if (cfg_put(cfg, t, &val, i, j))
		return 0;
	return -1; // Out of space
}

// Put zero-terminated string to the storage. If string length exceeds MAX_CFG_STR_LEN the -1 will be returned.
int cfg_put_str_(struct config* cfg, const struct cfg_type* t, unsigned char i, const char* str)
{
	BUG_ON(!t->str);
	BUG_ON(i && t->nind < 1);
	unsigned sz = strlen(str);
	if (sz > MAX_CFG_STR_LEN)
		return -1; // String too long
	if (cfg_put(cfg, t, str, i, sz))
		return 0;
	return -1; // Out of space
}

