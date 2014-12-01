#include "uart.h"
#include "aconv.h"

struct uart_ctx g_uart;

// Send current TX buffer asynchronously
void uart_send_start()
{
	g_uart.tx_curr = 0;
	IE2 |= UCA0TXIE;
}

static inline void uart_send_next()
{
	UCA0TXBUF = g_uart.tx_buff[g_uart.tx_curr];
	if (++g_uart.tx_curr >= g_uart.tx_len)
		IE2 &= ~UCA0TXIE;
}

// USCI A0/B0 Transmit ISR
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
	uart_send_next();
}

// Send current TX buffer asynchronously (no more than UART_BUFF_SZ)
void uart_send_str_async(const char* str)
{
	g_uart.tx_len = 0;
	for (; *str && g_uart.tx_len < UART_BUFF_SZ; ++str)
		g_uart.tx_buff[g_uart.tx_len++] = *str;
	uart_send_start();
}

// Send string to UART. Strings of length <= UART_BUFF_SZ
// will be sent without blocking.
void uart_send_str(const char* str)
{
	while (*str) {
		uart_wait();
		uart_send_str_async(str);
		str += g_uart.tx_len;
	}
}

// Formatted output
void uart_print_i(int n)
{
	uart_wait();
	g_uart.tx_len = i2a(n, g_uart.tx_buff);
	uart_send_start();
}

void uart_print_x(unsigned n)
{
	uart_wait();
	g_uart.tx_len = x2a(n, g_uart.tx_buff);
	uart_send_start();
}

// Start/stop receiving.
void uart_receive_enable(uart_rx_cb cb)
{
	g_uart.rx_len = 0;
	g_uart.rx_cb = cb;
	if (cb)
		IE2 |= UCA0RXIE;
	else
		IE2 &= ~UCA0RXIE;
}

static inline void uart_receive_next()
{
	char c = UCA0RXBUF;
	if (g_uart.rx_len < UART_BUFF_SZ)
		g_uart.rx_buff[g_uart.rx_len++] = c;
	if (g_uart.rx_cb)
		g_uart.rx_cb(c);
}

// USCI A0/B0 Receive ISR
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	uart_receive_next();
}

