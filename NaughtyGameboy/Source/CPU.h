#pragma once

#include "PCH.h"

// The Flag Register(lower 8bit of AF register)
// Bit  Name  Set Clr  Expl.
// 7    zf    Z   NZ   Zero Flag
// 6    n - -Add / Sub  Flag(BCD)
// 5    h - -Half Carry Flag(BCD)
// 4    cy    C   NC   Carry Flag
// 3 - 0 - --Not used(always zero)
#define ZeroFlag 7
#define SubtractFlag 6
#define HalfCarryFlag 5
#define CarryFlag 4

class CPU
{
private:
	ushort _AF; // Accumulator & Flags;
	ushort _BC; // General purpose
	ushort _DE; // General purpose
	ushort _HL; // General purpose
	ushort _SP; // Stack pointer
	ushort _PC; // Program counter

	byte* _byteRegisterMap[0x08]; // For easy access to the 8-bit registers (A, B, C, D, E, H, L). F
	ushort* _ushortRegisterMap[0x04]; // For easy access to the 16-bit registers (BC, DE, HL, SP)

	typedef ulong(CPU::*instructionFunction)(const byte& opCode);
	instructionFunction _instructionMap[0x100];
	instructionFunction _instructionMapCB[0x100];

public:
	CPU();
	~CPU();

private:
	// Instruction Set
	ulong NOP(const byte& opCode); // 0x00
};
