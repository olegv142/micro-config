#pragma once

#include "uart.h"

typedef enum {
	cli_ignored,
	cli_accepted,
	cli_done,
	cli_complete_pending,
	cli_failed
} cli_proc_res;

struct cli_subsystem;

typedef void (*cli_cb)();
typedef cli_proc_res (*cli_proc_cb)(int start);

struct cli_subsystem {
	cli_proc_cb           process;
	cli_cb                complete;
	cli_cb                get_help;
};

struct cli_ctx {
	const struct cli_subsystem* const* subsystems;
	const struct cli_subsystem* processing;
	const struct cli_subsystem* complete_pending;
	cli_proc_res                proc_res;
};

extern struct cli_ctx g_cli;

// Compare receiver buffer with the command string. Returns 1 if the buffer is the
// non-empty prefix of the command. Otherwise returns 0.
int cli_cmd_matched(const char* cmd);

// Asynchronous processing callback
static inline void cli_process()
{
	if (g_cli.complete_pending) {
		g_cli.complete_pending->complete(g_cli.complete_pending);
		g_cli.complete_pending = 0;
	}
}

// Initialize command line interface
void cli_init();

