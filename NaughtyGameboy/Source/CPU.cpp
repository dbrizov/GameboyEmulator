#include "CPU.h"
#include "Logger.h"

const byte CPU::ZeroFlag = 7;
const byte CPU::SubtractFlag = 6;
const byte CPU::HalfCarryFlag = 5;
const byte CPU::CarryFlag = 4;

CPU::CPU() :
	m_cycles(0),
	m_AF(0x0000),
	m_BC(0x0000),
	m_DE(0x0000),
	m_HL(0x0000),
	m_SP(0x0000),
	m_PC(0x0000)
{	
	// In binary ##dddsss, where ddd is the DST register, and sss is the SRC register
	// -------
	// B = 000
	// C = 001
	// D = 010
	// E = 011
	// H = 100
	// L = 101
	// F = 110 - unused
	// A = 111
	m_byteRegisterMap[0x00] = reinterpret_cast<byte*>(&m_BC) + 1; // Because the Z80 CPU is low-endian, a 16-bit address 0x[B][C] in the memory is [C][B]. The low byte comes first
	m_byteRegisterMap[0x01] = reinterpret_cast<byte*>(&m_BC);
	m_byteRegisterMap[0x02] = reinterpret_cast<byte*>(&m_DE) + 1;
	m_byteRegisterMap[0x03] = reinterpret_cast<byte*>(&m_DE);
	m_byteRegisterMap[0x04] = reinterpret_cast<byte*>(&m_HL) + 1;
	m_byteRegisterMap[0x05] = reinterpret_cast<byte*>(&m_HL);
	m_byteRegisterMap[0x06] = reinterpret_cast<byte*>(&m_AF); // Should not be used (F)
	m_byteRegisterMap[0x07] = reinterpret_cast<byte*>(&m_AF) + 1;
	
	// In binary ##rr####, where rr is a 16-bit register
	// -------
	// 00 = BC
	// 01 = DE
	// 10 = HL
	// 11 = SP
	m_ushortRegisterMap[0x00] = &m_BC;
	m_ushortRegisterMap[0x01] = &m_DE;
	m_ushortRegisterMap[0x02] = &m_HL;
	m_ushortRegisterMap[0x03] = &m_SP;

	m_MMU = std::make_unique<MMU>();

	InitInstructionMap();
}

ulong CPU::Step()
{
	ulong cycles = 0;

	ushort address = m_PC;
	byte opCode = ReadBytePCI();
	InstructionFunction instruction = nullptr;

	if (opCode == 0xCB)
	{
		opCode = ReadBytePCI();
		instruction = m_instructionMapCB[opCode];
	}
	else
	{
		instruction = m_instructionMap[opCode];
	}

	if (instruction != nullptr)
	{
		cycles = (this->*instruction)(opCode);
	}
	else
	{
		Logger::LogError("OpCode 0x%02X at address 0x%04X could not be interpreted.", opCode, address);
	}

	return cycles;
}

byte CPU::ReadBytePCI()
{
	byte value = m_MMU->ReadByte(m_PC);
	m_PC++;

	return value;
}

ushort CPU::ReadUShortPCI()
{
	ushort value = m_MMU->ReadUShort(m_PC);
	m_PC += 2;

	return value;
}

void CPU::InitInstructionMap()
{
	for (int i = 0; i < ARRAY_SIZE(m_instructionMap); i++)
	{
		m_instructionMap[i] = nullptr;
	}

	for (int i = 0; i < ARRAY_SIZE(m_instructionMapCB); i++)
	{
		m_instructionMapCB[i] = nullptr;
	}

	m_instructionMap[0x00] = &CPU::NOP;
}

ulong CPU::NOP(byte opCode)
{
	return 4;
}
