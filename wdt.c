#pragma once

#include "wdt.h"

struct wdt_ctx g_wdt;

#pragma vector=WDT_VECTOR
__interrupt void wdt_timer(void)
{
	struct wdt_clnt* c;
	++g_wdt.clk;
	for (c = g_wdt.clients; c; c = c->next)
		c->cb(g_wdt.clk);
}
