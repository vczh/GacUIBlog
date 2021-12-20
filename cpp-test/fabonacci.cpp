#include <iostream>
#include <algorithm>
#include <vector> 

using namespace std;

struct uint128_t
{
	uint64_t high = 0;
	uint64_t low = 0;
};

uint128_t Add(uint128_t a, uint128_t b)
{
	uint128_t c;
	c.high = a.high + b.high;
	c.low = a.low + b.low;
	if (c.low < a.low || c.low < b.low)
	{
		c.high += 1;
	}
	return c;
}

void Div10(uint128_t x, uint128_t& result, int& rem)
{
	/*
	Let 10 ax + ay = x.high
	Let 10 bx + by = x.low
	Let 10 cx + cy = 2^64
	(2^64 * x.high + x.low) / 10
	= (2^64 * (10 ax + ay) + (10 bx + by)) / 10
	= 2^64 ax + bx + (2^64 ay + by) / 10
	= 2^64 ax + bx + ((10 cx + cy) ay + by) / 10
	= 2^64 ax + bx + (10 cx ay + cy ay + by) / 10
	= 2^64 ax + bx + cx ay + (cy ay + by) / 10

	high = ax
	low = bx + cx ay + (cy ay + by) / 10
	rem = (cy ay + by) % 10
	*/
	uint64_t ax = x.high / 10;
	uint64_t ay = x.high % 10;
	uint64_t bx = x.low / 10;
	uint64_t by = x.low % 10;
	uint64_t cx = UINT64_MAX / 10;
	uint64_t cy = UINT64_MAX % 10 + 1; // not 10, no need to carry into cx

	result.high = ax;
	result.low = bx + cx * ay + (cy * ay + by) / 10;
	rem = (cy * ay + by) % 10;
}

void Print(uint128_t x)
{
	vector<char> digits;
	while (x.high != 0 || x.low != 0)
	{
		int rem = 0;
		Div10(x, x, rem);
		digits.push_back("0123456789"[rem]);
	}
	reverse(begin(digits), end(digits));
	cout << string(begin(digits), end(digits)) << endl;
}

int main()
{
	cout << "f(1) = " << 1 << endl;
	cout << "f(2) = " << 1 << endl;
	uint128_t a = { 0,1 };
	uint128_t b = { 0,1 };
	uint128_t c = { 0,0 };
	for (int i = 3; i <= 100; i++)
	{
		cout << "f(" << i << ") = ";
		Print(c = Add(a, b));
		a = b;
		b = c;
	}
	return 0;
}
