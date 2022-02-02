// string.c
#include "../localTypes.h"

/*e*/
s32
s2i(u8 *b)/*p;*/
{
	s32 result     = 0;
	s32 isNegative = 1;
	if (*b == '-')
	{
		b++;
		isNegative = -1;
	}
	
	if (*b == '0' && *(b+1) == 'x')
	{
		// process hex numbers
		b += 2;
		tryAnotherByte:
		if ( (*b >= '0') && (*b <= '9') )
		{
			result = (result * 16) + (*b - '0');
			b++;
			goto tryAnotherByte;
		} else if ( (*b >= 'A') && (*b <= 'F') ) {
			result = (result * 16) + (*b - ('A' - 10));
			b++;
			goto tryAnotherByte;
		} else {
			goto end;
		}
	}
	
	while( (*b >= '0') && (*b <= '9') )
	{
		result = (result * 10) + (*b - '0');
		b++;
	}
	
	end:
	result = isNegative * result;
	
	return result;
}

/*e*/
u8*
i2s(s32 in, u8 *out)/*p;*/
{
	u8 number[16];
	s32 remainder = 0;
	s32 index = 0;
	s32 isNegative = 0;
	if (in >> 31)
	{
		isNegative = 1;
		in = -in;
	}
	do {
		remainder = asmMod(in, 10);
		in        = asmGetDiv();
		number[index++] = remainder + '0';
	} while (in != 0);
	index--;
	// number is now consumed as a string
	if (isNegative)
	{
		*out++ = '-';
	}
	do {
		*out++ = number[index--];
	} while (index >= 0);
	*out = 0;
	return out;
}

/*e*/
u8*
i2sh(s32 in, u8 *out)/*p;*/
{
	// generate number directly in output
	s32 byte;
	for (s32 x = 0; x < 8; x++)
	{
		byte = (in>>((7-x)*4))&0x0F;
		byte += '0';
		if (byte > '9')
		{
			byte += 7;
		}
		out[x] = byte;
	}
	out += 8;
	*out = 0;
	return out;
}
