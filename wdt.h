#pragma once

#include "io430.h"

typedef void (*wdt_cb)(unsigned);

struct wdt_clnt {
	wdt_cb cb;
	struct wdt_clnt* next;
};

struct wdt_ctx {
	unsigned clk;
	struct wdt_clnt* clients;
};

extern struct wdt_ctx g_wdt;

static inline void wdt_timer_subscribe(struct wdt_clnt *c)
{
	c->next = g_wdt.clients;
	g_wdt.clients = c;
}

static inline void wdt_hold()
{
	// Stop watchdog timer to prevent time out reset
 	WDTCTL = WDTPW + WDTHOLD;
}

static inline void wdt_timer_init()
{
	// SMCLK / 32768 (32msec for 1MHz clock)
	WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL;
	IE1 |= WDTIE;
}
