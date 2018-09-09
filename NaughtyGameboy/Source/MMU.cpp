#include "MMU.h"
#include "BitUtil.h"

MMU::MMU()
{
	for (int i = 0; i < ARRAY_SIZE(m_memory); i++)
	{
		m_memory[i] = 0x00;
	}
}

byte MMU::ReadByte(ushort address)
{
	return m_memory[address];
}

void MMU::WriteByte(ushort address, byte value)
{
	m_memory[address] = value;
}

ushort MMU::ReadUShort(ushort address)
{
	// The lowByte comes first in memory, because the CPU is low-endian
	ushort lowByte = ReadByte(address);
	ushort highByte = ReadByte(address + 1);
	highByte = (highByte << 8);
	ushort value = highByte | lowByte;

	return value;
}

void MMU::WriteUShort(ushort address, ushort value)
{
	// The lowByte comes first in memory, because the CPU is low-endian
	byte lowByte = GetLowByte(value);
	WriteByte(address, lowByte);
	byte highByte = GetHighByte(value);
	WriteByte(address + 1, highByte);
}
