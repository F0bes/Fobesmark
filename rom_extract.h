#include <tamtypes.h>

// Thank you aap for providing this code
typedef struct ExpandState ExpandState;
struct ExpandState
{
	u32 size;
	u32 blockDesc;
	u32 n;
	u32 shift;
	u32 mask;
	u8 *ptr;
};
ExpandState exstate;

void ExpandInit(u8 *data)
{
	exstate.size = *(u32 *)data;
	exstate.ptr = data + 4;
}

void ExpandSetBlock(void)
{
	exstate.blockDesc = *exstate.ptr++;
	exstate.blockDesc = (exstate.blockDesc << 8) | *exstate.ptr++;
	exstate.blockDesc = (exstate.blockDesc << 8) | *exstate.ptr++;
	exstate.blockDesc = (exstate.blockDesc << 8) | *exstate.ptr++;
	exstate.n = exstate.blockDesc & 3;
	exstate.shift = 14 - exstate.n;
	exstate.mask = 0x3FFF >> exstate.n;
}

void ExpandMain(u8 *dst)
{
	int n, m;
	u8 b;
	u8 *start, *src;

	start = dst;
	for (n = 0; dst < start + exstate.size; n--, exstate.blockDesc <<= 1)
	{
		if (n == 0)
		{
			n = 30;
			ExpandSetBlock();
		}
		b = *exstate.ptr++;
		if (exstate.blockDesc & 0x80000000)
		{
			u16 h = (u16)b << 8 | *exstate.ptr++;
			src = dst - 1 - (h & exstate.mask);
			m = 2 + (h >> exstate.shift);
			*dst++ = *src++;
			while (m--)
				*dst++ = *src++;
		}
		else
			*dst++ = b;
	}
}

u32 Expand(u8 *src, u8 *dst)
{
	ExpandInit(src);
	ExpandMain(dst);
	return exstate.size;
}
