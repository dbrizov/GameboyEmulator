#pragma once

#include "PCH.h"

class CPU
{
private:
	//The Flag Register(lower 8bit of AF register)
	//	Bit  Name  Set Clr  Expl.
	//	7    zf    Z   NZ   Zero Flag
	//	6    n - -Add / Sub - Flag(BCD)
	//	5    h - -Half Carry Flag(BCD)
	//	4    cy    C   NC   Carry Flag
	//	3 - 0 - --Not used(always zero)

	ushort _AF; // Accumulator & Flags;
	ushort _BC; // General purpose
	ushort _DE; // General purpose
	ushort _HL; // General purpose
	ushort _SP; // Stack pointer
	ushort _PC; // Program counter

public:
};
