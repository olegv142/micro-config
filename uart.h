#pragma once
#include "io430.h"

#define UART_BUFF_SZ 32

typedef void (*uart_rx_cb)(char);

struct uart_ctx {
	char tx_buff[UART_BUFF_SZ];
	char rx_buff[UART_BUFF_SZ];
	unsigned char tx_len;
	unsigned char tx_curr;
	unsigned char rx_len;
	uart_rx_cb    rx_cb;
};

extern struct uart_ctx g_uart;

static inline void uart_init()
{
	// Configure UART
	P1SEL  |= BIT1|BIT2;    // P1.1=RXD P1.2=TXD
	P1SEL2 |= BIT1|BIT2;
	UCA0CTL1 |= UCSSEL_2;   // SMCLK
	UCA0BR0 = 104;          // 1MHz 9600
	UCA0BR1 = 0;
	UCA0MCTL = UCBRS0;      // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;   // **Initialize USCI state machine**
}

static inline void uart_wait()
{
	while (g_uart.tx_curr < g_uart.tx_len || (UCA0STAT & UCBUSY))
		__no_operation();
}

// Send current TX buffer asynchronously (no more than UART_BUFF_SZ)
void uart_send_start();

// Send string asynchronously.
void uart_send_str_async(const char* str);

// Send string to UART. Strings of length <= UART_BUFF_SZ
// will be sent without blocking.
void uart_send_str(const char* str);

// Formatted output
void uart_print_i(int n);
void uart_print_x(unsigned n);

// Start/stop receiving.
void uart_receive_enable(uart_rx_cb cb);
