#include "ver_cli.h"
#include "version.h"
#include "uart.h"

static cli_proc_res ver_cli_process(int start)
{
	if (cli_cmd_matched("version"))
		return cli_complete_pending;
	else
		return cli_ignored;
}

static void ver_cli_complete()
{
	uart_send_str("version "VER_STRING" ["VER_INFO"] built "__DATE__" "__TIME__"\r");
}

static void ver_cli_get_help()
{
	uart_send_str("version\t- show version info\r");
}

const struct cli_subsystem ver_cli = {
	ver_cli_process,
	ver_cli_complete,
	ver_cli_get_help
};


