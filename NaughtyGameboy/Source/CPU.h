#pragma once

#include "PCH.h"
#include "MMU.h"

class CPU
{
private:
	// The Flag Register(lower 8bit of AF register)
	// Bit  Name  Set Clr  Expl.
	// 7    Z     Z   NZ   Zero Flag
	// 6    N     -   -    Add / Sub - Flag (BCD)
	// 5    H     -   -    Half Carry Flag(BCD)
	// 4    C     C   NC   Carry Flag
	// 3-0  -     -   -    Not used(always zero)
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

	byte* m_byteRegisterMap[0x08]; // For easy access to the 8bit registers (A, B, C, D, E, H, L). F is not used
	ushort* m_ushortRegisterMap[0x04]; // For easy access to the 16bit registers (BC, DE, HL, SP)

	std::unique_ptr<MMU> m_MMU;

	typedef ulong(CPU::*InstructionFunction)(byte opcode);
	InstructionFunction m_instructionMap[0x100];
	InstructionFunction m_instructionMapCB[0x100];

public:
	CPU();

	/** Returns the number of cycles each step takes */
	ulong Step();

private:
	/** Read 1 byte and increment PC by 1 */
	byte ReadBytePCI();

	/** Read 2 bytes and increment PC by 2 */
	ushort ReadUShortPCI();

	void InitInstructionMap();

	/** Get an 8bit source register mapped to an opcode */
	byte* GetByteRegister_Src(byte opcode);

	/** Get an 8bit destination register mapped to an opcode  */
	byte* GetByteRegister_Dst(byte opcode);

	/** Get a 16bit register mapped to an opcode */
	ushort* GetUShortRegister(byte opcode);

	/** Push 1 byte to the stack */
	void PushByteToStack(byte value);

	/** Push 1 ushort to the stack */
	void PushUShortToStack(ushort value);

	/** Pop 1 byte from the stack */
	byte PopByteFromStack();

	/** Pop 1 ushort from the stack */
	ushort PopUShortFromStack();

	/** Adds 2 bytes and sets/clears the flags in the F register */
	byte AddByte(byte b1, byte b2);

	// ===============
	// INSTRUCTION SET
	// ===============
	// NOTE: 0x in the method names means indirect addressing.
	// 0xHL (HL) - the address pointed to by the HL register
	// 0xFF00 (FF00) - the memory address FF00
	// ===============

	// =======================
	// 8bit load instructions
	// =======================

	/** Load 8bit register R into 8bit register r */
	ulong LD_r_R(byte opcode);

	/** Load byte n into 8bit register r */
	ulong LD_r_n(byte opcode);

	/** Load the byte at address (HL) into 8bit register r */
	ulong LD_r_0xHL(byte opcode);

	/** Load 8bit register r into address (HL) */
	ulong LD_0xHL_r(byte opcode);

	/** Load byte n into address (HL) */
	ulong LD_0xHL_n(byte opcode);

	/** Load the byte at address (BC) into register A */
	ulong LD_A_0xBC(byte opcode);

	/** Load the byte at address (DE) into register A */
	ulong LD_A_0xDE(byte opcode);

	/** Load the byte at address (nn) into register A */
	ulong LD_A_0xnn(byte opcode);

	/** Load register A into address (BC) */
	ulong LD_0xBC_A(byte opcode);

	/** Load resiger A into address (DE) */
	ulong LD_0xDE_A(byte opcode);

	/** Load register A into address (nn) */
	ulong LD_0xnn_A(byte opcode);

	/** Read from IO port n (memory FF00+n) */
	ulong LD_A_0xFF00n(byte opcode);

	/** Write to IO port n (memory FF00+n) */
	ulong LD_0xFF00n_A(byte opcode);

	/** Read from IO port C (memory FF00+C) */
	ulong LD_A_0xFF00C(byte opcode);

	/** Write to IO port C (memory FF00+C) */
	ulong LD_0xFF00C_A(byte opcode);

	/** Load register A into address (HL), and increment HL */
	ulong LDI_0xHL_A(byte opcode);

	/** Load the byte at address (HL) into register A, and increment HL */
	ulong LDI_A_0xHL(byte opcode);

	/** Load register A into address (HL), and decrement HL */
	ulong LDD_0xHL_A(byte opcode);

	/** Load the byte at address (HL) into register A, and decrement HL */
	ulong LDD_A_0xHL(byte opcode);

	// =======================
	// 16bit load instruction
	// =======================

	/** Load ushort nn into 16bit register rr */
	ulong LD_rr_nn(byte opcode);

	/** Load register HL into register SP */
	ulong LD_SP_HL(byte opcode);

	/** Push 16bit register rr into the stack */
	ulong PUSH_rr(byte opcode);

	/** Pop 2 bytes from the stack and load them into 16bit register rr */
	ulong POP_rr(byte opcode);

	// =====================================
	// 8bit arithmetic/logical instructions
	// =====================================

	/** A = A + r (r - 8bit register) */
	ulong ADD_A_r(byte opcode);

	// ==================
	// Control instructions
	// ==================

	ulong NOP(byte opcode); // 0x00
};
