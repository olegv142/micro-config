//
// Configuration auto test
//

#include "io430.h"
#include "init.h"
#include "wdt.h"
#include "cli.h"
#include "uart.h"
#include "cfg.h"
#include "aconv.h"
#include "debug.h"
#include "uart.h"
#include "crc16.h"

#define M 16

unsigned ttl = 100;
unsigned last_clk;

// To simulate power failure we connect P1.6 to ground and
// pass VCC through 510 ohm resistor
void power_failure()
{
#ifdef SIMULATE_PW_FAIL
	P1DIR |= BIT6;
	P1OUT |= BIT6;
#endif
}

void wdtcb(unsigned clk)
{
	if (clk > ttl)
		dbg_reset();
	else if (clk >= ttl)
		power_failure();
	last_clk = clk;
}

struct wdt_clnt wdtc = {wdtcb};
int last_sys[M], last_usr[M];

static int verify_array(const struct cfg_type* ta)
{
	int i;
	int sys_min, sys_max, usr_min, usr_max;

	for (i = 0; i < M; ++i) {
		int sys = cfg_get_val_i(&cfg_system, ta, i);
		int usr = cfg_get_val_i(&cfg_user,   ta, i);
		last_sys[i] = sys;
		last_usr[i] = usr;
		if (!i) {
			sys_min = sys_max = sys;
			usr_min = usr_max = usr;
		} else {
			BUG_ON(sys - last_sys[i-1] > 0);
			BUG_ON(usr - last_usr[i-1] > 0);
			if (sys - sys_max > 0) sys_max = sys;
			if (sys - sys_min < 0) sys_min = sys;
			if (usr - usr_max > 0) usr_max = usr;
			if (usr - usr_min < 0) usr_min = usr;
		}
	}

	BUG_ON(sys_max - sys_min < 0);
	BUG_ON(sys_max - sys_min > 1);
	BUG_ON(usr_max - usr_min < 0);
	BUG_ON(usr_max - usr_min > 1);
	BUG_ON(sys_max != sys_min && usr_max != usr_min);
	BUG_ON(sys_max - usr_max > 1);
	BUG_ON(sys_max - usr_max < 0);

	if (sys_max != sys_min) {
		for (i = 0; i < M; ++i)
			cfg_put_val_i(&cfg_system, ta, i, sys_max);
	}
	if (usr_max != usr_min) {
		for (i = 0; i < M; ++i)
			cfg_put_val_i(&cfg_user, ta, i, usr_max);
	}
	if (sys_max - usr_max > 0) {
		for (i = 0; i < M; ++i)
			cfg_put_val_i(&cfg_user, ta, i, sys_max);
	}

	return sys_max;
}

static void update_array(const struct cfg_type* ta, int v)
{
	int i;
	for (i = 0; i < M; ++i)
		cfg_put_val_i(&cfg_system, ta, i, v);
	for (i = 0; i < M; ++i)
		cfg_put_val_i(&cfg_user, ta, i, v);
}

static void test_init()
{
	init_clock();
	wdt_timer_init();
	wdt_timer_subscribe(&wdtc);

	uart_init();

	__enable_interrupt();

	dbg_init();
	cfg_init_all();
	cli_init();
}

static void test_print(const char* str)
{
	uart_send_str(str);
	uart_send_str("\r");
	uart_wait();
}

void main( void )
{
	const struct cfg_type* t;
	const char* str;
	int v, n, sz;
	char buff[7];

	test_init();
	BUG_ON(crc16_str(CRC16_CHK_STR) != CRC16_CHK_VALUE);
	//
	// Update run counter
	//
	t = get_cfg_type("test.runs");
	BUG_ON(!t);
	n = cfg_get_val(&cfg_system, t);
	cfg_put_val(&cfg_system, t, n + 1);
	//
	// Testing string value
	//
	t = get_cfg_type("test.str");
	BUG_ON(!t);
	str = cfg_get_str_i(&cfg_system, t, 0);
	test_print(str);
	v = a2i(str);
	BUG_ON(v != n && v != n - 1);
	sz = i2az(n + 1, buff);
	BUG_ON(sz >= sizeof(buff));
	cfg_put_str_i(&cfg_system, t, 0, buff);
	//
	// The following value is written only once
	// It helps to test sequence numbering scheme
	//
	t = get_cfg_type("test.static");
	BUG_ON(!t);
	v = cfg_get_val(&cfg_system, t);
	if (!v) {
		BUG_ON(n);
		cfg_put_val(&cfg_system, t, 1);
	}
	//
	// Now testing value array
	//
	t = get_cfg_type("test.arr");
	BUG_ON(!t);
	v = verify_array(t);
	//
	// Schedule suicide
	//
	ttl = last_clk + ((unsigned)n * 27361) % 61;
	//
	// Update test array in a loop
	//
	for (;;)
	{
		cli_process();
		update_array(t, ++v);
		if (!(v & 0xf))
			cfg_init_all();
	}
}
