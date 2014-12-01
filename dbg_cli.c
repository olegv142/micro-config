#include "dbg_cli.h"
#include "debug.h"
#include "uart.h"

static void dbg_cli_complete()
{
	if (!dbg_soft_restarted()) {
		uart_send_str("no debug info available\r");
	} else {
		uart_send_str("was ");
		dbg_print_restart_info();
	}
}

static cli_proc_res dbg_cli_process(int start)
{
	if (cli_cmd_matched("reset"))
		dbg_reset();
	if (cli_cmd_matched("dbg"))
		return cli_complete_pending;
	return cli_ignored;
}

static void dbg_cli_get_help()
{
	uart_send_str("reset\t- reset device\r");
	uart_send_str("dbg\t- show debug info\r");
}

const struct cli_subsystem dbg_cli = {
	dbg_cli_process,
	dbg_cli_complete,
	dbg_cli_get_help
};
