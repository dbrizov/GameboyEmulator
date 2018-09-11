#pragma once

#include "PCH.h"

#define GET_BIT(value, bit) ((value >> bit) & 1)
#define SET_BIT(value, bit) (value | (1 << bit))
#define CLEAR_BIT(value, bit) (value & ~(1 << bit))
#define IS_BIT_SET(value, bit) (((value >> bit) & 1) == 1)

inline byte GetLowByte(ushort src)
{
	return (src & 0xFF);
}

inline byte GetHighByte(ushort src)
{
	return (src >> 8);
}

inline void SetLowByte(ushort* dst, byte value)
{
	ushort high = GetHighByte(*dst);
	*dst = (high << 8) | (ushort)value;
}

inline void SetHighByte(ushort* dst, byte value)
{
	ushort low = GetLowByte(*dst);
	*dst = (ushort)(value << 8) | low;
}
