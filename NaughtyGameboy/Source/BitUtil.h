#pragma once

#include "PCH.h"

inline bool IsBitSet(ushort value, ushort bit)
{
	return (((value >> bit) & 1) == 1);
}

inline ushort SetBit(ushort value, ushort bit)
{
	return (value | (1 << bit));
}

inline ushort ClearBit(ushort value, ushort bit)
{
	return (value & ~(1 << bit));
}

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
	byte high = GetHighByte(*dst);
	*dst = (high << 8) | value;
}

inline void SetHighByte(ushort* dst, byte value)
{
	byte low = GetLowByte(*dst);
	*dst = (value << 8) | low;
}
