#include "debug.h"
#include "uart.h"
#include "wdt.h"

#include "io430.h"

__no_init struct dbg_info g_dgb_info;

static inline void reset()
{
	FCTL2 = 0xDEAD;
}

int dbg_soft_restarted()
{
	return FCTL3 & KEYV;
}

static inline void clear_bug_info()
{
	g_dgb_info.bug_file = 0;
}

static inline void print_bug_info()
{
	uart_send_str("BUG at ");
	uart_send_str(g_dgb_info.bug_file);
	uart_send_str(":");
	uart_print_i(g_dgb_info.bug_line);
	uart_send_str("\r");
	uart_wait();
}

void dbg_print_restart_info()
{
	if (!g_dgb_info.bug_file)
		uart_send_str("restarted\r");
	else
		print_bug_info();
}

void dbg_init()
{
	if (dbg_soft_restarted())
		dbg_print_restart_info();
	else
		clear_bug_info();
}

void dbg_reset()
{
	clear_bug_info();
	reset();
}

void dbg_fatal(const char* file, int line)
{
	g_dgb_info.bug_file = file;
	g_dgb_info.bug_line = line;
	reset();
}
