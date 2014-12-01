#pragma once

struct dbg_info {
	const char* bug_file;
	int bug_line;
};

extern struct dbg_info g_dgb_info;

void dbg_init();
void dbg_reset();
void dbg_fatal(const char* file, int line);
int  dbg_soft_restarted();
void dbg_print_restart_info();

#define BUG()            do { dbg_fatal(__FILE__, __LINE__); } while(0)
#define BUG_ON(cond)     do { if (cond) dbg_fatal(__FILE__, __LINE__); } while(0)

#ifdef DBG_BUG_ON_EN
#define DBG_BUG_ON(cond) BUG_ON(cond)
#define DBG_BUG()        BUG()
#else
#define DBG_BUG()        do {} while(0)
#define DBG_BUG_ON(cond) do {} while(0)
#endif
