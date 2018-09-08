#pragma once

#include "PCH.h"

class MMU
{
private:
	byte m_memory[0xFFFF + 1];

public:
	MMU();

	byte ReadByte(ushort address);
	void WriteByte(ushort address, byte value);
	
	ushort ReadUShort(ushort address);
};
