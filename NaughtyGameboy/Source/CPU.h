#pragma once

#include "PCH.h"
#include "MMU.h"

class CPU
{
private:
	// The Flag Register(lower 8bit of AF register)
	// Bit  Name  Set Clr  Expl.
	// 7    zf    Z   NZ   Zero Flag
	// 6    n - -Add / Sub  Flag(BCD)
	// 5    h - -Half Carry Flag(BCD)
	// 4    cy    C   NC   Carry Flag
	// 3 - 0 - --Not used(always zero)
	static const byte ZeroFlag;
	static const byte SubtractFlag;
	static const byte HalfCarryFlag;
	static const byte CarryFlag;

private:
	ulong m_cycles; // Total cycles

	// Registers
	ushort m_AF; // Accumulator & Flags;
	ushort m_BC; // General purpose
	ushort m_DE; // General purpose
	ushort m_HL; // General purpose
	ushort m_SP; // Stack pointer
	ushort m_PC; // Program counter

	byte* m_byteRegisterMap[0x08]; // For easy access to the 8-bit registers (A, B, C, D, E, H, L). F is not used
	ushort* m_ushortRegisterMap[0x04]; // For easy access to the 16-bit registers (BC, DE, HL, SP)

	std::unique_ptr<MMU> m_MMU;

	typedef ulong(CPU::*InstructionFunction)(byte opCode);
	InstructionFunction m_instructionMap[0x100];
	InstructionFunction m_instructionMapCB[0x100];

public:
	CPU();

	/** Returns the number of cycles each step takes */
	ulong Step();

private:
	/** Read 1 byte and increment PC */
	byte ReadBytePCI();

	/** Read 2 bytes and increment PC */
	ushort ReadUShortPCI();

	void InitInstructionMap();

	// Instruction Set
	ulong NOP(byte opCode); // 0x00
};
