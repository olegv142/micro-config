#include "aconv.h"

int a2i(const char* str)
{
	int acc = 0, m = 0;
	if (str[0] == '-') {
		++str;
		m = 1;
	}
	for (;;) {
		char c = *str;
		if (c < '0' || c > '9')
			break;
		acc *= 10;
		acc += c - '0';
		++str;
	}
	return m ? -acc : acc;
}

int i2a(int n, char* buff)
{
	if (!n) {
		buff[0] = '0';
		return 1;
	}
	int m = 0, len = 0, i;
	if (n < 0) {
		*buff++ = '-';
		n = -n;
		m = 1;
	}
	while (n) {
		unsigned d = n % 10;
		n /= 10;
		for (i = len; i > 0; --i)
			buff[i] = buff[i-1];
		buff[0] = '0' + d;
		++len;
	}
	return m ? len + 1 : len;
}

int x2a(unsigned n, char* buff)
{
	if (!n) {
		buff[0] = '0';
		return 1;
	}
	int shift, len = 0;
	for (shift = 12; shift >= 0; shift -= 4) {
		unsigned d = (n >> shift) & 0xf;
		if (!d && !len)
			continue;
		*buff++ = d < 10 ? '0' + d : 'A' + (d - 10);
		++len;
	}
	return len;
}

