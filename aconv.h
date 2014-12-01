#pragma once

// ASCII <-> number conversions

int a2i(const char* str);
int i2a(int n, char* buff);
int x2a(unsigned n, char* buff);

static inline int i2az(int n, char* buff)
{
	int r = i2a(n, buff);
	buff[r] = 0;
	return r;
}

static inline int x2az(unsigned n, char* buff)
{
	int r = x2a(n, buff);
	buff[r] = 0;
	return r;
}
