#include "cli.h"
#include "ver_cli.h"
#include "dbg_cli.h"
#include "cfg_cli.h"

#define IS_EOL(ch)   (ch == '\r' || ch == '\n')
#define IS_SPACE(ch) (ch == ' '  || ch == '\t' || IS_EOL(ch))

// Compare receiver buffer with the command string. Returns 1 if the buffer is the
// non-empty prefix of the command. Otherwise returns 0.
int cli_cmd_matched(const char* cmd)
{
	const char* buff = g_uart.rx_buff;
	while (*buff && *cmd)
		if (*buff++ != *cmd++) // not matched
			return 0;
	if (*buff) // has extra symbols
		return 0;
	// matched
	return 1;
}

// Callback called on every symbol received by uart
static void cli_uart_cb(char ch)
{
	if (!IS_SPACE(ch))
		return;
	if (--g_uart.rx_len) // drop space
	{
		cli_proc_res res;
		// zero-terminate input
		g_uart.rx_buff[g_uart.rx_len] = 0;
		if (g_cli.complete_pending || g_cli.proc_res >= cli_done) {
			// unexpected input
			g_cli.proc_res = cli_failed;
			g_uart.rx_len = 0;
		}
		else if (g_cli.processing) {
			if ((res = g_cli.processing->process(0)) != cli_ignored) {
				g_cli.proc_res = res;
				g_uart.rx_len = 0;
			}
		}
		else {
			const struct cli_subsystem* const* ssp;
			for (ssp = g_cli.subsystems; *ssp; ++ssp) {
				const struct cli_subsystem* ss = *ssp;
				if ((res = ss->process(1)) != cli_ignored) {
					g_cli.processing = ss;
					g_cli.proc_res = res;
					g_uart.rx_len = 0;
					break;
				}
			}
		}
	}
	if (IS_EOL(ch)) {
		if (g_cli.proc_res == cli_accepted || (g_cli.proc_res == cli_ignored && g_uart.rx_len)) // incomplete input
			g_cli.proc_res = cli_failed;
		if (g_cli.proc_res == cli_failed)
			uart_send_str_async("invalid command\r");
		else if (g_cli.proc_res == cli_complete_pending)
			g_cli.complete_pending = g_cli.processing;
		// reset state
		g_cli.processing = 0;
		g_cli.proc_res = cli_ignored;
		g_uart.rx_len = 0;
	}
}

static cli_proc_res cli_help_process(int start)
{
	if (cli_cmd_matched("help"))
		return cli_complete_pending;
	else
		return cli_ignored;
}

static void cli_help_complete()
{
	const struct cli_subsystem* const* ssp;
	for (ssp = g_cli.subsystems; *ssp; ++ssp) {
		const struct cli_subsystem* ss = *ssp;
		ss->get_help();
	}
}

static void cli_help_get_help()
{
	uart_send_str("help\t- this help\r");
}

static const struct cli_subsystem hlp_cli = {
	cli_help_process,
	cli_help_complete,
	cli_help_get_help
};

static const struct cli_subsystem* const cli_subsystems[] = {
	&ver_cli,
	&cfg_cli,
	&dbg_cli,
	&hlp_cli,
	0
};

struct cli_ctx g_cli = {cli_subsystems};

void cli_init()
{
	uart_receive_enable(cli_uart_cb);
}

