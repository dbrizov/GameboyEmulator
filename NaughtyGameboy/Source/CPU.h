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

	typedef ulong(CPU::*InstructionFunction)(byte opcode);
	InstructionFunction m_instructionMap[0x100];
	InstructionFunction m_instructionMapCB[0x100];

public:
	CPU();

	/** Returns the number of cycles each step takes */
	ulong Step();

private:
	/** Read 1 byte and increment the PC by 1 */
	byte ReadBytePCI();

	/** Read 2 bytes and increment the PC by 2 */
	ushort ReadUShortPCI();

	void InitInstructionMap();

	/** Get an 8-bit source register mapped to an opcode */
	byte* GetByteRegister_Src(byte opcode);

	/** Get an 8-bit destination register mapped to an opcode  */
	byte* GetByteRegister_Dst(byte opcode);

	/** Get a 16-bit register mapped to an opcode */
	ushort* GetUShortRegister(byte opcode);

	// =======================
	// 8-bit load instructions
	// =======================

	/** Load 8-bit register 'R' into 8-bit register 'r' */
	ulong LD_r_R(byte opcode);

	/** Load byte 'n' into 8-bit register 'r' */
	ulong LD_r_n(byte opcode);

	/** Load the byte at address (HL) into 8-bit register 'r' */
	ulong LD_r_HL(byte opcode);

	/** Load 8-bit register 'r' into address (HL) */
	ulong LD_HL_r(byte opcode);

	/** Load byte 'n' into address (HL) */
	ulong LD_HL_n(byte opcode);

	/** Load the byte at address (BC) into register 'A' */
	ulong LD_A_BC(byte opcode);

	/** Load the byte at address (DE) into register 'A' */
	ulong LD_A_DE(byte opcode);

	/** Load the byte at address (nn) into register 'A' */
	ulong LD_A_nn(byte opcode);

	// ==================
	// Other instructions
	// ==================

	ulong NOP(byte opcode); // 0x00
};
