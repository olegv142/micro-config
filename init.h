#pragma once

#include "io430.h"

static inline void init_clock()
{
	// MCLK=SMCLK=1MHz
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;
}

