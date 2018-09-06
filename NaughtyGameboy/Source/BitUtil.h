#pragma once

#include "PCH.h"

inline bool IsBitSet(ushort val, ushort bit)
{
	return (((val >> bit) & 1) == 1);
}

inline ushort SetBit(ushort val, ushort bit)
{
	return (val | (1 << bit));
}

inline ushort ClearBit(ushort val, ushort bit)
{
	return (val & ~(1 << bit));
}

inline byte GetLowByte(ushort src)
{
	return (src & 0xFF);
}

inline byte GetHighByte(ushort src)
{
	return (src >> 8);
}

inline void SetLowByte(ushort* dst, byte val)
{
	byte high = GetHighByte(*dst);
	*dst = (high << 8) | val;
}

inline void SetHighByte(ushort* dst, byte val)
{
	byte low = GetLowByte(*dst);
	*dst = (val << 8) | low;
}
