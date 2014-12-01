#include "cfg.h"
#include "cfg_cli.h"
#include "cfg_types.h"
#include "debug.h"
#include "uart.h"
#include "aconv.h"
#include "string.h"

static void (*cfg_cli_complete_cb)() = 0;

#define BUFF_SZ MAX_CFG_STR_LEN

struct cfg_set_ctx {
	struct config* cfg;
	struct cfg_type const* type;
	unsigned char ind[2];
	unsigned char nind;
	union {
		int val;
		char buff[BUFF_SZ+1];
	};
};

static struct cfg_set_ctx set_ctx;

static void print_cfg_ind(int i)
{
	uart_send_str("[");
	uart_print_i(i);
	uart_send_str("]");
}

static void print_cfg_val(int val)
{
	uart_send_str("=");
	uart_print_i(val);
	uart_send_str("\r");
}

static void print_cfg_str(const char* str)
{
	uart_send_str("='");
	uart_send_str(str);
	uart_send_str("'\r");
}

static void print_cfg_types()
{
	const struct cfg_type* t;
	for (t = g_cfg_types; t->name; ++t)
	{
		int i;
		uart_send_str(t->name);
		for (i = 0; i < t->nind; ++i)
			uart_send_str("[]");
		if (t->str)
			print_cfg_str(def_cfg_str(t));
		else
			print_cfg_val(def_cfg_val(t));
	}
}

static void print_cfg_info(const struct config* cfg)
{
	uart_send_str("[");
	uart_print_i(cfg->area[0].used_bytes);
	uart_send_str("/");
	uart_print_i(cfg->area[1].used_bytes);
	uart_send_str(" bytes used, active area #");
	uart_print_i(cfg->active_area);
	uart_send_str(" is ");
	uart_send_str(cfg_area_status_names[cfg->area[cfg->active_area].status]);
	uart_send_str(", inactive area #");
	uart_print_i(cfg->active_area^1);
	uart_send_str(" is ");
	uart_send_str(cfg_area_status_names[cfg->area[cfg->active_area^1].status]);
	uart_send_str("]\r");
}

static void print_cfg_value_cb(struct config* cfg, struct cfg_type const* t, struct cfg_obj_header const* h, struct cfg_obj_footer const* f)
{
	uart_send_str(t->name);
	if (t->nind)
		print_cfg_ind(h->i);
	if (t->nind > 1)
		print_cfg_ind(h->j_sz);
	if (t->str)
		print_cfg_str((const char*)(h+1));
	else
		print_cfg_val(*(int*)(h+1));
}

static void print_cfg_values()
{
	uart_send_str("--- system config:\r");
	print_cfg_info(&cfg_system);
	cfg_enum(&cfg_system, print_cfg_value_cb);
	uart_send_str("--- user config:\r");
	print_cfg_info(&cfg_user);
	cfg_enum(&cfg_user, print_cfg_value_cb);
}

static void cfg_cli_set_complete()
{
	int i;
	struct cfg_set_ctx s = set_ctx;
	if (!s.cfg || !s.type)
		return;

	// store value
	int r;
	if (s.type->str)
		r = cfg_put_str_(s.cfg, s.type, s.ind[0], s.buff);
	else
		r = cfg_put_val_(s.cfg, s.type, s.ind[0], s.ind[1], s.val);
	if (r < 0) {
		uart_send_str("failed to store value");
		return;
	}

	// lookup value
	uart_send_str(s.type->name);
	for (i = 0; i < s.type->nind; ++i)
		print_cfg_ind(s.ind[i]);
	if (s.type->str)
		print_cfg_str(cfg_get_str_(s.cfg, s.type, s.ind[0]));
	else
		print_cfg_val(cfg_get_val_(s.cfg, s.type, s.ind[0], s.ind[1]));
}

static cli_proc_res cfg_cli_set(struct config* cfg)
{
	if (cfg) {
		// initialize
		set_ctx.cfg = cfg;
		set_ctx.type = 0;
		set_ctx.ind[0] = set_ctx.ind[1] = set_ctx.nind = 0;
		return cli_accepted;
	}
	if (!set_ctx.type) {
		// lookup type
		set_ctx.type = get_cfg_type(g_uart.rx_buff);
		return set_ctx.type ? cli_accepted : cli_failed;
	}
	if (set_ctx.nind < set_ctx.type->nind) {
		// get index
		set_ctx.ind[set_ctx.nind++] = a2i(g_uart.rx_buff);
		return cli_accepted;
	} else {
		// copy value
		if (set_ctx.type->str) {
			strncpy(set_ctx.buff, g_uart.rx_buff, BUFF_SZ);
		} else {
			set_ctx.val = a2i(g_uart.rx_buff);
		}
		cfg_cli_complete_cb = cfg_cli_set_complete;
		return cli_complete_pending;
	}
}

static cli_proc_res cfg_cli_process(int start)
{
	if (!start) {
		return cfg_cli_set(0);
	}
	if (cli_cmd_matched("set")) {
		return cfg_cli_set(&cfg_system);
	}
	if (cli_cmd_matched("setu")) {
		return cfg_cli_set(&cfg_user);
	}
	if (cli_cmd_matched("types")) {
		cfg_cli_complete_cb = print_cfg_types;
		return cli_complete_pending;
	}
	if (cli_cmd_matched("cfg")) {
		cfg_cli_complete_cb = print_cfg_values;
		return cli_complete_pending;
	}
	return cli_ignored;
}

static void cfg_cli_complete()
{
	BUG_ON(!cfg_cli_complete_cb);
	cfg_cli_complete_cb();
}

static void cfg_cli_get_help()
{
	uart_send_str("types\t- show configuration types\r");
	uart_send_str("cfg\t- show stored configuration\r");
	uart_send_str("set  name [i [j]] value - set system configuration value\r");
	uart_send_str("setu name [i [j]] value - set user   configuration value\r");
}

const struct cli_subsystem cfg_cli = {
	cfg_cli_process,
	cfg_cli_complete,
	cfg_cli_get_help
};


