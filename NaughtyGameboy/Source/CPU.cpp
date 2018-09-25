#include "CPU.h"
#include "Logger.h"
#include "BitUtil.h"

const byte CPU::ZeroFlag = 7;
const byte CPU::SubtractFlag = 6;
const byte CPU::HalfCarryFlag = 5;
const byte CPU::CarryFlag = 4;

const byte CPU::ZeroFlagMask = 1 << 7;
const byte CPU::SubtractFlagMask = 1 << 6;
const byte CPU::HalfCarryFlagMask = 1 << 5;
const byte CPU::CarryFlagMask = 1 << 4;
const byte CPU::AllFlagsMask = 0xF0;

CPU::CPU() :
	m_cycles(0),
	m_isHalted(false),
	m_IME(0),
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
	m_byteRegisterMap[0x00] = reinterpret_cast<byte*>(&m_BC) + 1; // Because the Z80 CPU is low-endian, a 16bit address 0x[B][C] in the memory is [C][B]. The low byte comes first
	m_byteRegisterMap[0x01] = reinterpret_cast<byte*>(&m_BC);
	m_byteRegisterMap[0x02] = reinterpret_cast<byte*>(&m_DE) + 1;
	m_byteRegisterMap[0x03] = reinterpret_cast<byte*>(&m_DE);
	m_byteRegisterMap[0x04] = reinterpret_cast<byte*>(&m_HL) + 1;
	m_byteRegisterMap[0x05] = reinterpret_cast<byte*>(&m_HL);
	m_byteRegisterMap[0x06] = reinterpret_cast<byte*>(&m_AF); // Should not be used (F)
	m_byteRegisterMap[0x07] = reinterpret_cast<byte*>(&m_AF) + 1;

	// In binary ##rr####, where rr is a 16bit register
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

	if (m_isHalted)
	{
		cycles = NOP(0x00);
	}
	else
	{
		ushort address = m_PC;
		byte opcode = ReadBytePCI();
		InstructionFunction instruction = nullptr;

		if (opcode == 0xCB)
		{
			opcode = ReadBytePCI();
			instruction = m_instructionMapCB[opcode];
		}
		else
		{
			instruction = m_instructionMap[opcode];
		}

		if (instruction != nullptr)
		{
			cycles = (this->*instruction)(opcode);
		}
		else
		{
			Logger::LogError("OpCode 0x%02X at address 0x%04X could not be interpreted.", opcode, address);
		}
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
	// 0x
	m_instructionMap[0x00] = &CPU::NOP;
	m_instructionMap[0x01] = &CPU::LD_rr_nn;
	m_instructionMap[0x02] = &CPU::LD_0xBC_A;
	m_instructionMap[0x03] = &CPU::INC_rr;
	m_instructionMap[0x04] = &CPU::INC_r;
	m_instructionMap[0x05] = &CPU::DEC_r;
	m_instructionMap[0x06] = &CPU::LD_r_n;
	m_instructionMap[0x07] = &CPU::RLCA;
	m_instructionMap[0x08] = &CPU::LD_0xnn_SP;
	m_instructionMap[0x09] = &CPU::ADD_HL_rr;
	m_instructionMap[0x0A] = &CPU::LD_A_0xBC;
	m_instructionMap[0x0B] = &CPU::DEC_rr;
	m_instructionMap[0x0C] = &CPU::INC_r;
	m_instructionMap[0x0D] = &CPU::DEC_r;
	m_instructionMap[0x0E] = &CPU::LD_r_n;
	m_instructionMap[0x0F] = &CPU::RRCA;

	// 1x
	m_instructionMap[0x10] = &CPU::STOP;
	m_instructionMap[0x11] = &CPU::LD_rr_nn;
	m_instructionMap[0x12] = &CPU::LD_0xDE_A;
	m_instructionMap[0x13] = &CPU::INC_rr;
	m_instructionMap[0x14] = &CPU::INC_r;
	m_instructionMap[0x15] = &CPU::DEC_r;
	m_instructionMap[0x16] = &CPU::LD_r_n;
	m_instructionMap[0x17] = &CPU::RLA;
	m_instructionMap[0x18] = &CPU::JR_dd;
	m_instructionMap[0x19] = &CPU::ADD_HL_rr;
	m_instructionMap[0x1A] = &CPU::LD_A_0xDE;
	m_instructionMap[0x1B] = &CPU::DEC_rr;
	m_instructionMap[0x1C] = &CPU::INC_r;
	m_instructionMap[0x1D] = &CPU::DEC_r;
	m_instructionMap[0x1E] = &CPU::LD_r_n;
	m_instructionMap[0x1F] = &CPU::RRA;

	// 2x
	m_instructionMap[0x20] = &CPU::JR_cc_dd;
	m_instructionMap[0x21] = &CPU::LD_rr_nn;
	m_instructionMap[0x22] = &CPU::LDI_0xHL_A;
	m_instructionMap[0x23] = &CPU::INC_rr;
	m_instructionMap[0x24] = &CPU::INC_r;
	m_instructionMap[0x25] = &CPU::DEC_r;
	m_instructionMap[0x26] = &CPU::LD_r_n;
	m_instructionMap[0x27] = &CPU::DAA;
	m_instructionMap[0x28] = &CPU::JR_cc_dd;
	m_instructionMap[0x29] = &CPU::ADD_HL_rr;
	m_instructionMap[0x2A] = &CPU::LDI_A_0xHL;
	m_instructionMap[0x2B] = &CPU::DEC_rr;
	m_instructionMap[0x2C] = &CPU::INC_r;
	m_instructionMap[0x2D] = &CPU::DEC_r;
	m_instructionMap[0x2E] = &CPU::LD_r_n;
	m_instructionMap[0x2F] = &CPU::CPL;

	// 3x
	m_instructionMap[0x30] = &CPU::JR_cc_dd;
	m_instructionMap[0x31] = &CPU::LD_rr_nn;
	m_instructionMap[0x32] = &CPU::LDD_0xHL_A;
	m_instructionMap[0x33] = &CPU::INC_rr;
	m_instructionMap[0x34] = &CPU::INC_0xHL;
	m_instructionMap[0x35] = &CPU::DEC_0xHL;
	m_instructionMap[0x36] = &CPU::LD_0xHL_n;
	m_instructionMap[0x37] = &CPU::SCF;
	m_instructionMap[0x38] = &CPU::JR_cc_dd;
	m_instructionMap[0x39] = &CPU::ADD_HL_rr;
	m_instructionMap[0x3A] = &CPU::LDD_A_0xHL;
	m_instructionMap[0x3B] = &CPU::DEC_rr;
	m_instructionMap[0x3C] = &CPU::INC_r;
	m_instructionMap[0x3D] = &CPU::DEC_r;
	m_instructionMap[0x3E] = &CPU::LD_r_n;
	m_instructionMap[0x3F] = &CPU::CCF;

	// 4x
	m_instructionMap[0x40] = &CPU::LD_r_R;
	m_instructionMap[0x41] = &CPU::LD_r_R;
	m_instructionMap[0x42] = &CPU::LD_r_R;
	m_instructionMap[0x43] = &CPU::LD_r_R;
	m_instructionMap[0x44] = &CPU::LD_r_R;
	m_instructionMap[0x45] = &CPU::LD_r_R;
	m_instructionMap[0x46] = &CPU::LD_r_0xHL;
	m_instructionMap[0x47] = &CPU::LD_r_R;
	m_instructionMap[0x48] = &CPU::LD_r_R;
	m_instructionMap[0x49] = &CPU::LD_r_R;
	m_instructionMap[0x4A] = &CPU::LD_r_R;
	m_instructionMap[0x4B] = &CPU::LD_r_R;
	m_instructionMap[0x4C] = &CPU::LD_r_R;
	m_instructionMap[0x4D] = &CPU::LD_r_R;
	m_instructionMap[0x4E] = &CPU::LD_r_0xHL;
	m_instructionMap[0x4F] = &CPU::LD_r_R;

	// 5x
	m_instructionMap[0x50] = &CPU::LD_r_R;
	m_instructionMap[0x51] = &CPU::LD_r_R;
	m_instructionMap[0x52] = &CPU::LD_r_R;
	m_instructionMap[0x53] = &CPU::LD_r_R;
	m_instructionMap[0x54] = &CPU::LD_r_R;
	m_instructionMap[0x55] = &CPU::LD_r_R;
	m_instructionMap[0x56] = &CPU::LD_r_0xHL;
	m_instructionMap[0x57] = &CPU::LD_r_R;
	m_instructionMap[0x58] = &CPU::LD_r_R;
	m_instructionMap[0x59] = &CPU::LD_r_R;
	m_instructionMap[0x5A] = &CPU::LD_r_R;
	m_instructionMap[0x5B] = &CPU::LD_r_R;
	m_instructionMap[0x5C] = &CPU::LD_r_R;
	m_instructionMap[0x5D] = &CPU::LD_r_R;
	m_instructionMap[0x5E] = &CPU::LD_r_0xHL;
	m_instructionMap[0x5F] = &CPU::LD_r_R;

	// 6x
	m_instructionMap[0x60] = &CPU::LD_r_R;
	m_instructionMap[0x61] = &CPU::LD_r_R;
	m_instructionMap[0x62] = &CPU::LD_r_R;
	m_instructionMap[0x63] = &CPU::LD_r_R;
	m_instructionMap[0x64] = &CPU::LD_r_R;
	m_instructionMap[0x65] = &CPU::LD_r_R;
	m_instructionMap[0x66] = &CPU::LD_r_0xHL;
	m_instructionMap[0x67] = &CPU::LD_r_R;
	m_instructionMap[0x68] = &CPU::LD_r_R;
	m_instructionMap[0x69] = &CPU::LD_r_R;
	m_instructionMap[0x6A] = &CPU::LD_r_R;
	m_instructionMap[0x6B] = &CPU::LD_r_R;
	m_instructionMap[0x6C] = &CPU::LD_r_R;
	m_instructionMap[0x6D] = &CPU::LD_r_R;
	m_instructionMap[0x6E] = &CPU::LD_r_0xHL;
	m_instructionMap[0x6F] = &CPU::LD_r_R;

	// 7x
	m_instructionMap[0x70] = &CPU::LD_0xHL_r;
	m_instructionMap[0x71] = &CPU::LD_0xHL_r;
	m_instructionMap[0x72] = &CPU::LD_0xHL_r;
	m_instructionMap[0x73] = &CPU::LD_0xHL_r;
	m_instructionMap[0x74] = &CPU::LD_0xHL_r;
	m_instructionMap[0x75] = &CPU::LD_0xHL_r;
	m_instructionMap[0x76] = &CPU::HALT;
	m_instructionMap[0x77] = &CPU::LD_0xHL_r;
	m_instructionMap[0x78] = &CPU::LD_r_R;
	m_instructionMap[0x79] = &CPU::LD_r_R;
	m_instructionMap[0x7A] = &CPU::LD_r_R;
	m_instructionMap[0x7B] = &CPU::LD_r_R;
	m_instructionMap[0x7C] = &CPU::LD_r_R;
	m_instructionMap[0x7D] = &CPU::LD_r_R;
	m_instructionMap[0x7E] = &CPU::LD_r_0xHL;
	m_instructionMap[0x7F] = &CPU::LD_r_R;

	// 8x
	m_instructionMap[0x80] = &CPU::ADD_A_r;
	m_instructionMap[0x81] = &CPU::ADD_A_r;
	m_instructionMap[0x82] = &CPU::ADD_A_r;
	m_instructionMap[0x83] = &CPU::ADD_A_r;
	m_instructionMap[0x84] = &CPU::ADD_A_r;
	m_instructionMap[0x85] = &CPU::ADD_A_r;
	m_instructionMap[0x86] = &CPU::ADD_A_0xHL;
	m_instructionMap[0x87] = &CPU::ADD_A_r;
	m_instructionMap[0x88] = &CPU::ADC_A_r;
	m_instructionMap[0x89] = &CPU::ADC_A_r;
	m_instructionMap[0x8A] = &CPU::ADC_A_r;
	m_instructionMap[0x8B] = &CPU::ADC_A_r;
	m_instructionMap[0x8C] = &CPU::ADC_A_r;
	m_instructionMap[0x8D] = &CPU::ADC_A_r;
	m_instructionMap[0x8E] = &CPU::ADC_A_0xHL;
	m_instructionMap[0x8F] = &CPU::ADC_A_r;

	// 9x
	m_instructionMap[0x90] = &CPU::SUB_A_r;
	m_instructionMap[0x91] = &CPU::SUB_A_r;
	m_instructionMap[0x92] = &CPU::SUB_A_r;
	m_instructionMap[0x93] = &CPU::SUB_A_r;
	m_instructionMap[0x94] = &CPU::SUB_A_r;
	m_instructionMap[0x95] = &CPU::SUB_A_r;
	m_instructionMap[0x96] = &CPU::SUB_A_0xHL;
	m_instructionMap[0x97] = &CPU::SUB_A_r;
	m_instructionMap[0x98] = &CPU::SBC_A_r;
	m_instructionMap[0x99] = &CPU::SBC_A_r;
	m_instructionMap[0x9A] = &CPU::SBC_A_r;
	m_instructionMap[0x9B] = &CPU::SBC_A_r;
	m_instructionMap[0x9C] = &CPU::SBC_A_r;
	m_instructionMap[0x9D] = &CPU::SBC_A_r;
	m_instructionMap[0x9E] = &CPU::SBC_A_0xHL;
	m_instructionMap[0x9F] = &CPU::SBC_A_r;

	// Ax
	m_instructionMap[0xA0] = &CPU::AND_r;
	m_instructionMap[0xA1] = &CPU::AND_r;
	m_instructionMap[0xA2] = &CPU::AND_r;
	m_instructionMap[0xA3] = &CPU::AND_r;
	m_instructionMap[0xA4] = &CPU::AND_r;
	m_instructionMap[0xA5] = &CPU::AND_r;
	m_instructionMap[0xA6] = &CPU::AND_0xHL;
	m_instructionMap[0xA7] = &CPU::AND_r;
	m_instructionMap[0xA8] = &CPU::XOR_r;
	m_instructionMap[0xA9] = &CPU::XOR_r;
	m_instructionMap[0xAA] = &CPU::XOR_r;
	m_instructionMap[0xAB] = &CPU::XOR_r;
	m_instructionMap[0xAC] = &CPU::XOR_r;
	m_instructionMap[0xAD] = &CPU::XOR_r;
	m_instructionMap[0xAE] = &CPU::XOR_0xHL;
	m_instructionMap[0xAF] = &CPU::XOR_r;

	// Bx
	m_instructionMap[0xB0] = &CPU::OR_r;
	m_instructionMap[0xB1] = &CPU::OR_r;
	m_instructionMap[0xB2] = &CPU::OR_r;
	m_instructionMap[0xB3] = &CPU::OR_r;
	m_instructionMap[0xB4] = &CPU::OR_r;
	m_instructionMap[0xB5] = &CPU::OR_r;
	m_instructionMap[0xB6] = &CPU::OR_0xHL;
	m_instructionMap[0xB7] = &CPU::OR_r;
	m_instructionMap[0xB8] = &CPU::CP_r;
	m_instructionMap[0xB9] = &CPU::CP_r;
	m_instructionMap[0xBA] = &CPU::CP_r;
	m_instructionMap[0xBB] = &CPU::CP_r;
	m_instructionMap[0xBC] = &CPU::CP_r;
	m_instructionMap[0xBD] = &CPU::CP_r;
	m_instructionMap[0xBE] = &CPU::CP_0xHL;
	m_instructionMap[0xBF] = &CPU::CP_r;

	// Cx
	m_instructionMap[0xC0] = &CPU::RET_cc;
	m_instructionMap[0xC1] = &CPU::POP_rr;
	m_instructionMap[0xC2] = &CPU::JP_cc_nn;
	m_instructionMap[0xC3] = &CPU::JP_nn;
	m_instructionMap[0xC4] = &CPU::CALL_cc_nn;
	m_instructionMap[0xC5] = &CPU::PUSH_rr;
	m_instructionMap[0xC6] = &CPU::ADD_A_n;
	m_instructionMap[0xC7] = &CPU::RST_n;
	m_instructionMap[0xC8] = &CPU::RET_cc;
	m_instructionMap[0xC9] = &CPU::RET;
	m_instructionMap[0xCA] = &CPU::JP_cc_nn;
	m_instructionMap[0xCB] = nullptr; // Prefix to use 0xCB instruction map (m_instructionMapCB)
	m_instructionMap[0xCC] = &CPU::CALL_cc_nn;
	m_instructionMap[0xCD] = &CPU::CALL_nn;
	m_instructionMap[0xCE] = &CPU::ADC_A_n;
	m_instructionMap[0xCF] = &CPU::RST_n;

	// Dx
	m_instructionMap[0xD0] = &CPU::RET_cc;
	m_instructionMap[0xD1] = &CPU::POP_rr;
	m_instructionMap[0xD2] = &CPU::JP_cc_nn;
	m_instructionMap[0xD3] = nullptr;
	m_instructionMap[0xD4] = &CPU::CALL_cc_nn;
	m_instructionMap[0xD5] = &CPU::PUSH_rr;
	m_instructionMap[0xD6] = &CPU::SUB_A_n;
	m_instructionMap[0xD7] = &CPU::RST_n;
	m_instructionMap[0xD8] = &CPU::RET_cc;
	m_instructionMap[0xD9] = &CPU::RETI;
	m_instructionMap[0xDA] = &CPU::JP_cc_nn;
	m_instructionMap[0xDB] = nullptr;
	m_instructionMap[0xDC] = &CPU::CALL_cc_nn;
	m_instructionMap[0xDD] = nullptr;
	m_instructionMap[0xDE] = &CPU::SBC_A_n;
	m_instructionMap[0xDF] = &CPU::RST_n;

	// Ex
	m_instructionMap[0xE0] = &CPU::LD_0xFF00n_A;
	m_instructionMap[0xE1] = &CPU::POP_rr;
	m_instructionMap[0xE2] = &CPU::LD_0xFF00C_A;
	m_instructionMap[0xE3] = nullptr;
	m_instructionMap[0xE4] = nullptr;
	m_instructionMap[0xE5] = &CPU::PUSH_rr;
	m_instructionMap[0xE6] = &CPU::AND_n;
	m_instructionMap[0xE7] = &CPU::RST_n;
	m_instructionMap[0xE8] = &CPU::ADD_SP_dd;
	m_instructionMap[0xE9] = &CPU::JP_HL;
	m_instructionMap[0xEA] = &CPU::LD_0xnn_A;
	m_instructionMap[0xEB] = nullptr;
	m_instructionMap[0xEC] = nullptr;
	m_instructionMap[0xED] = nullptr;
	m_instructionMap[0xEE] = &CPU::XOR_n;
	m_instructionMap[0xEF] = &CPU::RST_n;

	// Fx
	m_instructionMap[0xF0] = &CPU::LD_A_0xFF00n;
	m_instructionMap[0xF1] = &CPU::POP_rr;
	m_instructionMap[0xF2] = &CPU::LD_A_0xFF00C;
	m_instructionMap[0xF3] = &CPU::DI;
	m_instructionMap[0xF4] = nullptr;
	m_instructionMap[0xF5] = &CPU::PUSH_rr;
	m_instructionMap[0xF6] = &CPU::OR_n;
	m_instructionMap[0xF7] = &CPU::RST_n;
	m_instructionMap[0xF8] = &CPU::LD_HL_SPdd;
	m_instructionMap[0xF9] = &CPU::LD_SP_HL;
	m_instructionMap[0xFA] = &CPU::LD_A_0xnn;
	m_instructionMap[0xFB] = &CPU::EI;
	m_instructionMap[0xFC] = nullptr;
	m_instructionMap[0xFD] = nullptr;
	m_instructionMap[0xFE] = &CPU::CP_n;
	m_instructionMap[0xFF] = &CPU::RST_n;

	// =========
	// Prefix CB
	// =========

	// 0x
	m_instructionMapCB[0x00] = &CPU::RLC_r;
	m_instructionMapCB[0x01] = &CPU::RLC_r;
	m_instructionMapCB[0x02] = &CPU::RLC_r;
	m_instructionMapCB[0x03] = &CPU::RLC_r;
	m_instructionMapCB[0x04] = &CPU::RLC_r;
	m_instructionMapCB[0x05] = &CPU::RLC_r;
	m_instructionMapCB[0x06] = &CPU::RLC_0xHL;
	m_instructionMapCB[0x07] = &CPU::RLC_r;
	m_instructionMapCB[0x08] = &CPU::RRC_r;
	m_instructionMapCB[0x09] = &CPU::RRC_r;
	m_instructionMapCB[0x0A] = &CPU::RRC_r;
	m_instructionMapCB[0x0B] = &CPU::RRC_r;
	m_instructionMapCB[0x0C] = &CPU::RRC_r;
	m_instructionMapCB[0x0D] = &CPU::RRC_r;
	m_instructionMapCB[0x0E] = &CPU::RRC_0xHL;
	m_instructionMapCB[0x0F] = &CPU::RRC_r;

	// 1x
	m_instructionMapCB[0x10] = &CPU::RL_r;
	m_instructionMapCB[0x11] = &CPU::RL_r;
	m_instructionMapCB[0x12] = &CPU::RL_r;
	m_instructionMapCB[0x13] = &CPU::RL_r;
	m_instructionMapCB[0x14] = &CPU::RL_r;
	m_instructionMapCB[0x15] = &CPU::RL_r;
	m_instructionMapCB[0x16] = &CPU::RL_0xHL;
	m_instructionMapCB[0x17] = &CPU::RL_r;
	m_instructionMapCB[0x18] = &CPU::RR_r;
	m_instructionMapCB[0x19] = &CPU::RR_r;
	m_instructionMapCB[0x1A] = &CPU::RR_r;
	m_instructionMapCB[0x1B] = &CPU::RR_r;
	m_instructionMapCB[0x1C] = &CPU::RR_r;
	m_instructionMapCB[0x1D] = &CPU::RR_r;
	m_instructionMapCB[0x1E] = &CPU::RR_0xHL;
	m_instructionMapCB[0x1F] = &CPU::RR_r;

	// 2x
	m_instructionMapCB[0x20] = &CPU::SLA_r;
	m_instructionMapCB[0x21] = &CPU::SLA_r;
	m_instructionMapCB[0x22] = &CPU::SLA_r;
	m_instructionMapCB[0x23] = &CPU::SLA_r;
	m_instructionMapCB[0x24] = &CPU::SLA_r;
	m_instructionMapCB[0x25] = &CPU::SLA_r;
	m_instructionMapCB[0x26] = &CPU::SLA_0xHL;
	m_instructionMapCB[0x27] = &CPU::SLA_r;
	m_instructionMapCB[0x28] = &CPU::SRA_r;
	m_instructionMapCB[0x29] = &CPU::SRA_r;
	m_instructionMapCB[0x2A] = &CPU::SRA_r;
	m_instructionMapCB[0x2B] = &CPU::SRA_r;
	m_instructionMapCB[0x2C] = &CPU::SRA_r;
	m_instructionMapCB[0x2D] = &CPU::SRA_r;
	m_instructionMapCB[0x2E] = &CPU::SRA_0xHL;
	m_instructionMapCB[0x2F] = &CPU::SRA_r;

	// 3x
	m_instructionMapCB[0x30] = &CPU::SWAP_r;
	m_instructionMapCB[0x31] = &CPU::SWAP_r;
	m_instructionMapCB[0x32] = &CPU::SWAP_r;
	m_instructionMapCB[0x33] = &CPU::SWAP_r;
	m_instructionMapCB[0x34] = &CPU::SWAP_r;
	m_instructionMapCB[0x35] = &CPU::SWAP_r;
	m_instructionMapCB[0x36] = &CPU::SWAP_0xHL;
	m_instructionMapCB[0x37] = &CPU::SWAP_r;
	m_instructionMapCB[0x38] = &CPU::SRL_r;
	m_instructionMapCB[0x39] = &CPU::SRL_r;
	m_instructionMapCB[0x3A] = &CPU::SRL_r;
	m_instructionMapCB[0x3B] = &CPU::SRL_r;
	m_instructionMapCB[0x3C] = &CPU::SRL_r;
	m_instructionMapCB[0x3D] = &CPU::SRL_r;
	m_instructionMapCB[0x3E] = &CPU::SRL_0xHL;
	m_instructionMapCB[0x3F] = &CPU::SRL_r;

	// 4x
	m_instructionMapCB[0x40] = &CPU::BIT_n_r;
	m_instructionMapCB[0x41] = &CPU::BIT_n_r;
	m_instructionMapCB[0x42] = &CPU::BIT_n_r;
	m_instructionMapCB[0x43] = &CPU::BIT_n_r;
	m_instructionMapCB[0x44] = &CPU::BIT_n_r;
	m_instructionMapCB[0x45] = &CPU::BIT_n_r;
	m_instructionMapCB[0x46] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x47] = &CPU::BIT_n_r;
	m_instructionMapCB[0x48] = &CPU::BIT_n_r;
	m_instructionMapCB[0x49] = &CPU::BIT_n_r;
	m_instructionMapCB[0x4A] = &CPU::BIT_n_r;
	m_instructionMapCB[0x4B] = &CPU::BIT_n_r;
	m_instructionMapCB[0x4C] = &CPU::BIT_n_r;
	m_instructionMapCB[0x4D] = &CPU::BIT_n_r;
	m_instructionMapCB[0x4E] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x4F] = &CPU::BIT_n_r;

	// 5x
	m_instructionMapCB[0x50] = &CPU::BIT_n_r;
	m_instructionMapCB[0x51] = &CPU::BIT_n_r;
	m_instructionMapCB[0x52] = &CPU::BIT_n_r;
	m_instructionMapCB[0x53] = &CPU::BIT_n_r;
	m_instructionMapCB[0x54] = &CPU::BIT_n_r;
	m_instructionMapCB[0x55] = &CPU::BIT_n_r;
	m_instructionMapCB[0x56] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x57] = &CPU::BIT_n_r;
	m_instructionMapCB[0x58] = &CPU::BIT_n_r;
	m_instructionMapCB[0x59] = &CPU::BIT_n_r;
	m_instructionMapCB[0x5A] = &CPU::BIT_n_r;
	m_instructionMapCB[0x5B] = &CPU::BIT_n_r;
	m_instructionMapCB[0x5C] = &CPU::BIT_n_r;
	m_instructionMapCB[0x5D] = &CPU::BIT_n_r;
	m_instructionMapCB[0x5E] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x5F] = &CPU::BIT_n_r;

	// 6x
	m_instructionMapCB[0x60] = &CPU::BIT_n_r;
	m_instructionMapCB[0x61] = &CPU::BIT_n_r;
	m_instructionMapCB[0x62] = &CPU::BIT_n_r;
	m_instructionMapCB[0x63] = &CPU::BIT_n_r;
	m_instructionMapCB[0x64] = &CPU::BIT_n_r;
	m_instructionMapCB[0x65] = &CPU::BIT_n_r;
	m_instructionMapCB[0x66] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x67] = &CPU::BIT_n_r;
	m_instructionMapCB[0x68] = &CPU::BIT_n_r;
	m_instructionMapCB[0x69] = &CPU::BIT_n_r;
	m_instructionMapCB[0x6A] = &CPU::BIT_n_r;
	m_instructionMapCB[0x6B] = &CPU::BIT_n_r;
	m_instructionMapCB[0x6C] = &CPU::BIT_n_r;
	m_instructionMapCB[0x6D] = &CPU::BIT_n_r;
	m_instructionMapCB[0x6E] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x6F] = &CPU::BIT_n_r;

	// 7x
	m_instructionMapCB[0x70] = &CPU::BIT_n_r;
	m_instructionMapCB[0x71] = &CPU::BIT_n_r;
	m_instructionMapCB[0x72] = &CPU::BIT_n_r;
	m_instructionMapCB[0x73] = &CPU::BIT_n_r;
	m_instructionMapCB[0x74] = &CPU::BIT_n_r;
	m_instructionMapCB[0x75] = &CPU::BIT_n_r;
	m_instructionMapCB[0x76] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x77] = &CPU::BIT_n_r;
	m_instructionMapCB[0x78] = &CPU::BIT_n_r;
	m_instructionMapCB[0x79] = &CPU::BIT_n_r;
	m_instructionMapCB[0x7A] = &CPU::BIT_n_r;
	m_instructionMapCB[0x7B] = &CPU::BIT_n_r;
	m_instructionMapCB[0x7C] = &CPU::BIT_n_r;
	m_instructionMapCB[0x7D] = &CPU::BIT_n_r;
	m_instructionMapCB[0x7E] = &CPU::BIT_n_0xHL;
	m_instructionMapCB[0x7F] = &CPU::BIT_n_r;

	// 8x
	m_instructionMapCB[0x80] = &CPU::RES_n_r;
	m_instructionMapCB[0x81] = &CPU::RES_n_r;
	m_instructionMapCB[0x82] = &CPU::RES_n_r;
	m_instructionMapCB[0x83] = &CPU::RES_n_r;
	m_instructionMapCB[0x84] = &CPU::RES_n_r;
	m_instructionMapCB[0x85] = &CPU::RES_n_r;
	m_instructionMapCB[0x86] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0x87] = &CPU::RES_n_r;
	m_instructionMapCB[0x88] = &CPU::RES_n_r;
	m_instructionMapCB[0x89] = &CPU::RES_n_r;
	m_instructionMapCB[0x8A] = &CPU::RES_n_r;
	m_instructionMapCB[0x8B] = &CPU::RES_n_r;
	m_instructionMapCB[0x8C] = &CPU::RES_n_r;
	m_instructionMapCB[0x8D] = &CPU::RES_n_r;
	m_instructionMapCB[0x8E] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0x8F] = &CPU::RES_n_r;

	// 9x
	m_instructionMapCB[0x90] = &CPU::RES_n_r;
	m_instructionMapCB[0x91] = &CPU::RES_n_r;
	m_instructionMapCB[0x92] = &CPU::RES_n_r;
	m_instructionMapCB[0x93] = &CPU::RES_n_r;
	m_instructionMapCB[0x94] = &CPU::RES_n_r;
	m_instructionMapCB[0x95] = &CPU::RES_n_r;
	m_instructionMapCB[0x96] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0x97] = &CPU::RES_n_r;
	m_instructionMapCB[0x98] = &CPU::RES_n_r;
	m_instructionMapCB[0x99] = &CPU::RES_n_r;
	m_instructionMapCB[0x9A] = &CPU::RES_n_r;
	m_instructionMapCB[0x9B] = &CPU::RES_n_r;
	m_instructionMapCB[0x9C] = &CPU::RES_n_r;
	m_instructionMapCB[0x9D] = &CPU::RES_n_r;
	m_instructionMapCB[0x9E] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0x9F] = &CPU::RES_n_r;

	// Ax
	m_instructionMapCB[0xA0] = &CPU::RES_n_r;
	m_instructionMapCB[0xA1] = &CPU::RES_n_r;
	m_instructionMapCB[0xA2] = &CPU::RES_n_r;
	m_instructionMapCB[0xA3] = &CPU::RES_n_r;
	m_instructionMapCB[0xA4] = &CPU::RES_n_r;
	m_instructionMapCB[0xA5] = &CPU::RES_n_r;
	m_instructionMapCB[0xA6] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0xA7] = &CPU::RES_n_r;
	m_instructionMapCB[0xA8] = &CPU::RES_n_r;
	m_instructionMapCB[0xA9] = &CPU::RES_n_r;
	m_instructionMapCB[0xAA] = &CPU::RES_n_r;
	m_instructionMapCB[0xAB] = &CPU::RES_n_r;
	m_instructionMapCB[0xAC] = &CPU::RES_n_r;
	m_instructionMapCB[0xAD] = &CPU::RES_n_r;
	m_instructionMapCB[0xAE] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0xAF] = &CPU::RES_n_r;

	// Bx
	m_instructionMapCB[0xB0] = &CPU::RES_n_r;
	m_instructionMapCB[0xB1] = &CPU::RES_n_r;
	m_instructionMapCB[0xB2] = &CPU::RES_n_r;
	m_instructionMapCB[0xB3] = &CPU::RES_n_r;
	m_instructionMapCB[0xB4] = &CPU::RES_n_r;
	m_instructionMapCB[0xB5] = &CPU::RES_n_r;
	m_instructionMapCB[0xB6] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0xB7] = &CPU::RES_n_r;
	m_instructionMapCB[0xB8] = &CPU::RES_n_r;
	m_instructionMapCB[0xB9] = &CPU::RES_n_r;
	m_instructionMapCB[0xBA] = &CPU::RES_n_r;
	m_instructionMapCB[0xBB] = &CPU::RES_n_r;
	m_instructionMapCB[0xBC] = &CPU::RES_n_r;
	m_instructionMapCB[0xBD] = &CPU::RES_n_r;
	m_instructionMapCB[0xBE] = &CPU::RES_n_0xHL;
	m_instructionMapCB[0xBF] = &CPU::RES_n_r;

	// Cx
	m_instructionMapCB[0xC0] = &CPU::SET_n_r;
	m_instructionMapCB[0xC1] = &CPU::SET_n_r;
	m_instructionMapCB[0xC2] = &CPU::SET_n_r;
	m_instructionMapCB[0xC3] = &CPU::SET_n_r;
	m_instructionMapCB[0xC4] = &CPU::SET_n_r;
	m_instructionMapCB[0xC5] = &CPU::SET_n_r;
	m_instructionMapCB[0xC6] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xC7] = &CPU::SET_n_r;
	m_instructionMapCB[0xC8] = &CPU::SET_n_r;
	m_instructionMapCB[0xC9] = &CPU::SET_n_r;
	m_instructionMapCB[0xCA] = &CPU::SET_n_r;
	m_instructionMapCB[0xCB] = &CPU::SET_n_r;
	m_instructionMapCB[0xCC] = &CPU::SET_n_r;
	m_instructionMapCB[0xCD] = &CPU::SET_n_r;
	m_instructionMapCB[0xCE] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xCF] = &CPU::SET_n_r;

	// Dx
	m_instructionMapCB[0xD0] = &CPU::SET_n_r;
	m_instructionMapCB[0xD1] = &CPU::SET_n_r;
	m_instructionMapCB[0xD2] = &CPU::SET_n_r;
	m_instructionMapCB[0xD3] = &CPU::SET_n_r;
	m_instructionMapCB[0xD4] = &CPU::SET_n_r;
	m_instructionMapCB[0xD5] = &CPU::SET_n_r;
	m_instructionMapCB[0xD6] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xD7] = &CPU::SET_n_r;
	m_instructionMapCB[0xD8] = &CPU::SET_n_r;
	m_instructionMapCB[0xD9] = &CPU::SET_n_r;
	m_instructionMapCB[0xDA] = &CPU::SET_n_r;
	m_instructionMapCB[0xDB] = &CPU::SET_n_r;
	m_instructionMapCB[0xDC] = &CPU::SET_n_r;
	m_instructionMapCB[0xDD] = &CPU::SET_n_r;
	m_instructionMapCB[0xDE] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xDF] = &CPU::SET_n_r;

	// Ex
	m_instructionMapCB[0xE0] = &CPU::SET_n_r;
	m_instructionMapCB[0xE1] = &CPU::SET_n_r;
	m_instructionMapCB[0xE2] = &CPU::SET_n_r;
	m_instructionMapCB[0xE3] = &CPU::SET_n_r;
	m_instructionMapCB[0xE4] = &CPU::SET_n_r;
	m_instructionMapCB[0xE5] = &CPU::SET_n_r;
	m_instructionMapCB[0xE6] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xE7] = &CPU::SET_n_r;
	m_instructionMapCB[0xE8] = &CPU::SET_n_r;
	m_instructionMapCB[0xE9] = &CPU::SET_n_r;
	m_instructionMapCB[0xEA] = &CPU::SET_n_r;
	m_instructionMapCB[0xEB] = &CPU::SET_n_r;
	m_instructionMapCB[0xEC] = &CPU::SET_n_r;
	m_instructionMapCB[0xED] = &CPU::SET_n_r;
	m_instructionMapCB[0xEE] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xEF] = &CPU::SET_n_r;

	// Fx
	m_instructionMapCB[0xF0] = &CPU::SET_n_r;
	m_instructionMapCB[0xF1] = &CPU::SET_n_r;
	m_instructionMapCB[0xF2] = &CPU::SET_n_r;
	m_instructionMapCB[0xF3] = &CPU::SET_n_r;
	m_instructionMapCB[0xF4] = &CPU::SET_n_r;
	m_instructionMapCB[0xF5] = &CPU::SET_n_r;
	m_instructionMapCB[0xF6] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xF7] = &CPU::SET_n_r;
	m_instructionMapCB[0xF8] = &CPU::SET_n_r;
	m_instructionMapCB[0xF9] = &CPU::SET_n_r;
	m_instructionMapCB[0xFA] = &CPU::SET_n_r;
	m_instructionMapCB[0xFB] = &CPU::SET_n_r;
	m_instructionMapCB[0xFC] = &CPU::SET_n_r;
	m_instructionMapCB[0xFD] = &CPU::SET_n_r;
	m_instructionMapCB[0xFE] = &CPU::SET_n_0xHL;
	m_instructionMapCB[0xFF] = &CPU::SET_n_r;
}

byte* CPU::GetByteRegister_Src(byte opcode)
{
	return m_byteRegisterMap[opcode & 0x07];
}

byte* CPU::GetByteRegister_Dst(byte opcode)
{
	return m_byteRegisterMap[(opcode >> 3) & 0x07];
}

ushort* CPU::GetUShortRegister(byte opcode)
{
	// TODO Need to check this. There are some special cases with the PUSH and POP instruction that the AF register is used. However I think this is a Z80 thing only
	return m_ushortRegisterMap[(opcode >> 4) & 0x03];
}

void CPU::PushByteToStack(byte value)
{
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to push something to the stack we need to decrement the stack pointer first
	m_SP--;
	m_MMU->WriteByte(m_SP, value);
}

void CPU::PushUShortToStack(ushort value)
{
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to push something to the stack we need to decrement the stack pointer first
	m_SP -= 2;
	m_MMU->WriteUShort(m_SP, value);
}

byte CPU::PopByteFromStack()
{
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to pop something from the stack we need to increase the stack pointer after we read the data from it
	byte value = m_MMU->ReadByte(m_SP);
	m_SP++;

	return value;
}

ushort CPU::PopUShortFromStack()
{
	// The stack is in range FF80-FFFE where FFFE is the bottom of the stack, and FF80 is the maximum top of the stack
	// So in order to pop something from the stack we need to increase the stack pointer after we read the data from it
	ushort value = m_MMU->ReadUShort(m_SP);
	m_SP += 2;

	return value;
}

byte CPU::GetFlag(byte flag)
{
	byte F = GetLowByte(m_AF);
	return GET_BIT(F, flag);
}

void CPU::SetFlag(byte flag)
{
	byte F = GetLowByte(m_AF);
	F = SET_BIT(F, flag);
	SetLowByte(&m_AF, F);
}

void CPU::ClearFlag(byte flag)
{
	byte F = GetLowByte(m_AF);
	F = CLEAR_BIT(F, flag);
	SetLowByte(&m_AF, F);
}

bool CPU::IsFlagSet(byte flag)
{
	byte F = GetLowByte(m_AF);
	return IS_BIT_SET(F, flag);
}

byte CPU::AddBytes_Two(byte b1, byte b2, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 + b2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		ClearFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		(((b1 & 0x0F) + (b2 & 0x0F)) > 0x0F) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 + b2) > 0xFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::AddBytes_Three(byte b1, byte b2, byte b3, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 + b2 + b3;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		ClearFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		(((b1 & 0x0F) + (b2 & 0x0F) + (b3 & 0x0F)) > 0x0F) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 + b2 + b3) > 0xFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

ushort CPU::AddUShorts_Two(ushort s1, ushort s2, byte affectedFlags /*= AllFlagsMask*/)
{
	ushort result = s1 + s2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x0000) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		ClearFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		(((s1 & 0x0F00) + (s2 & 0x0F00)) > 0x0F00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(s1 + s2) > 0xFFFF) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::SubtractBytes_Two(byte b1, byte b2, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 - b2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		SetFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		((int)((b1 & 0x0F) - (b2 & 0x0F)) < 0x00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 - b2) < 0x00) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::SubtractBytes_Three(byte b1, byte b2, byte b3, byte affectedFlags /*= AllFlagsMask*/)
{
	byte result = b1 - b2 - b3;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		SetFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		((int)((b1 & 0x0F) - (b2 & 0x0F) - (b3 & 0x0F)) < 0x00) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(b1 - b2 - b3) < 0x00) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

ushort CPU::SubtractUShorts_Two(ushort s1, ushort s2, byte affectedFlags /*= AllFlagsMask*/)
{
	ushort result = s1 - s2;

	if (IS_BIT_SET(affectedFlags, ZeroFlag))
	{
		(result == 0x0000) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	if (IS_BIT_SET(affectedFlags, SubtractFlag))
	{
		SetFlag(SubtractFlag);
	}

	if (IS_BIT_SET(affectedFlags, HalfCarryFlag))
	{
		((int)((s1 & 0x0F00) - (s2 & 0x0F00)) < 0x0000) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	}

	if (IS_BIT_SET(affectedFlags, CarryFlag))
	{
		((int)(s1 - s2) < 0x0000) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);
	}

	return result;
}

byte CPU::CompareBytes(byte b1, byte b2)
{
	byte flags = 0x00;

	if (b1 == b2)
	{
		flags = SET_BIT(flags, ZeroFlag);
	}

	flags = SET_BIT(flags, SubtractFlag);

	if ((int)((b1 & 0x0F) - (b2 - 0x0F)) < 0x00)
	{
		flags = SET_BIT(flags, HalfCarryFlag);
	}

	if ((int)(b1 - b2) < 0x00)
	{
		flags = SET_BIT(flags, CarryFlag);
	}

	return flags;
}

byte CPU::RotateLeft(byte b, bool clearZeroFlag /*= false*/)
{
	byte newCF = GET_BIT(b, 7);
	b = b << 1;
	b = (newCF == 1) ? SET_BIT(b, 0) : CLEAR_BIT(b, 0);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
}

byte CPU::RotateLeftThroughCarry(byte b, bool clearZeroFlag /*= false*/)
{
	byte oldCF = GetFlag(CarryFlag);
	byte newCF = GET_BIT(b, 7);
	b = b << 1;
	b = (oldCF == 1) ? SET_BIT(b, 0) : CLEAR_BIT(b, 0);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
}

byte CPU::RotateRight(byte b, bool clearZeroFlag /*= false*/)
{
	byte newCF = GET_BIT(b, 0);
	b = b >> 1;
	b = (newCF == 1) ? SET_BIT(b, 7) : CLEAR_BIT(b, 7);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
}

byte CPU::RotateRightThroughCarry(byte b, bool clearZeroFlag /*= false*/)
{
	byte oldCF = GetFlag(CarryFlag);
	byte newCF = GET_BIT(b, 0);
	b = b >> 1;
	b = (oldCF == 1) ? SET_BIT(b, 7) : CLEAR_BIT(b, 7);

	if (clearZeroFlag)
	{
		ClearFlag(ZeroFlag);
	}
	else
	{
		(b == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	}

	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(newCF == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return b;
}

bool CPU::OpcodeCondition(byte opcode)
{
	// ###cc###
	// Not Zero(cc = 00)
	// Zero(cc = 01)
	// Not Carry(cc = 10)
	// Carry(cc = 11)
	byte cc = (opcode >> 3) & 0x03;
	bool conditionMet = false;

	switch (cc)
	{
	case 0x00:  // Not Zero
		conditionMet = !IsFlagSet(ZeroFlag);
		break;
	case 0x01:  // Zero
		conditionMet = IsFlagSet(ZeroFlag);
		break;
	case 0x02:  // Not Carry
		conditionMet = !IsFlagSet(CarryFlag);
		break;
	case 0x03:  // Carry
		conditionMet = IsFlagSet(CarryFlag);
		break;
	}

	return conditionMet;
}

ulong CPU::LD_r_n(byte opcode)
{
	byte n = ReadBytePCI();
	byte* r = GetByteRegister_Dst(opcode);
	*r = n;

	return 8;
}

ulong CPU::LD_r_R(byte opcode)
{
	byte* R = GetByteRegister_Src(opcode);
	byte* r = GetByteRegister_Dst(opcode);
	*r = *R;

	return 4;
}

ulong CPU::LD_r_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte* r = GetByteRegister_Dst(opcode);
	*r = value;

	return 8;
}

ulong CPU::LD_0xHL_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	m_MMU->WriteByte(m_HL, *r);

	return 8;
}

ulong CPU::LD_0xHL_n(byte opcode)
{
	byte n = ReadBytePCI();
	m_MMU->WriteByte(m_HL, n);

	return 12;
}

ulong CPU::LD_A_0xBC(byte opcode)
{
	byte value = m_MMU->ReadByte(m_BC);
	SetHighByte(&m_AF, value);

	return 8;
}

ulong CPU::LD_A_0xDE(byte opcode)
{
	byte value = m_MMU->ReadByte(m_DE);
	SetHighByte(&m_AF, value);

	return 8;
}

ulong CPU::LD_A_0xnn(byte opcode)
{
	ushort nn = ReadUShortPCI();
	byte value = m_MMU->ReadByte(nn);
	SetHighByte(&m_AF, value);

	return 16;
}

ulong CPU::LD_0xBC_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_BC, A);

	return 8;
}

ulong CPU::LD_0xDE_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_DE, A);

	return 8;
}

ulong CPU::LD_0xnn_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	ushort nn = ReadUShortPCI();
	m_MMU->WriteByte(nn, A);

	return 16;
}

ulong CPU::LD_A_0xFF00n(byte opcode)
{
	byte n = ReadBytePCI();
	byte value = m_MMU->ReadByte(0xFF00 + n);
	SetHighByte(&m_AF, value);

	return 12;
}

ulong CPU::LD_0xFF00n_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	m_MMU->WriteByte(0xFF00 + n, A);

	return 12;
}

ulong CPU::LD_A_0xFF00C(byte opcode)
{
	byte C = GetLowByte(m_BC);
	byte value = m_MMU->ReadByte(0xFF00 + C);
	SetHighByte(&m_AF, value);

	return 8;
}

ulong CPU::LD_0xFF00C_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte C = GetLowByte(m_BC);
	m_MMU->WriteByte(0xFF00 + C, A);

	return 8;
}

ulong CPU::LDI_0xHL_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_HL, A);
	m_HL++;

	return 8;
}

ulong CPU::LDI_A_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	SetHighByte(&m_AF, value);
	m_HL++;

	return 8;
}

ulong CPU::LDD_0xHL_A(byte opcode)
{
	byte A = GetHighByte(m_AF);
	m_MMU->WriteByte(m_HL, A);
	m_HL--;

	return 8;
}

ulong CPU::LDD_A_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	SetHighByte(&m_AF, value);
	m_HL--;

	return 8;
}

ulong CPU::LD_0xnn_SP(byte opcode)
{
	ushort nn = ReadUShortPCI();
	m_MMU->WriteUShort(nn, m_SP);

	return 20;
}

ulong CPU::LD_rr_nn(byte opcode)
{
	ushort nn = ReadUShortPCI();
	ushort* rr = GetUShortRegister(opcode);
	*rr = nn;

	return 12;
}

ulong CPU::LD_SP_HL(byte opcode)
{
	m_SP = m_HL;

	return 8;
}

ulong CPU::PUSH_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	PushUShortToStack(*rr);

	return 16;
}

ulong CPU::POP_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort value = PopUShortFromStack();
	*rr = value;

	return 12;
}

ulong CPU::ADD_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = AddBytes_Two(A, *r);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::ADD_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = AddBytes_Two(A, n);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADD_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = AddBytes_Two(A, value);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADC_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes_Three(A, *r, cf);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::ADC_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes_Three(A, n, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::ADC_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GetFlag(CarryFlag);
	byte result = AddBytes_Three(A, value, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SUB_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = SubtractBytes_Two(A, *r);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::SUB_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = SubtractBytes_Two(A, n);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SUB_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = SubtractBytes_Two(A, value);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SBC_A_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes_Three(A, *r, cf);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::SBC_A_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes_Three(A, n, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::SBC_A_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GetFlag(CarryFlag);
	byte result = SubtractBytes_Three(A, value, cf);
	SetHighByte(&m_AF, result);

	return 8;
}

ulong CPU::AND_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = A & *r;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 4;
}

ulong CPU::AND_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = A & n;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::AND_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = A & value;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::XOR_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = A ^ *r;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 4;
}

ulong CPU::XOR_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = A ^ n;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::XOR_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = A ^ value;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::OR_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte result = A | *r;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 4;
}

ulong CPU::OR_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte result = A | n;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::OR_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte result = A | value;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::CP_r(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte* r = GetByteRegister_Src(opcode);
	byte flags = CompareBytes(A, *r);
	SetLowByte(&m_AF, flags);

	return 4;
}

ulong CPU::CP_n(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte n = ReadBytePCI();
	byte flags = CompareBytes(A, n);
	SetLowByte(&m_AF, flags);

	return 8;
}

ulong CPU::CP_0xHL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte value = m_MMU->ReadByte(m_HL);
	byte flags = CompareBytes(A, value);
	SetLowByte(&m_AF, flags);

	return 8;
}

ulong CPU::INC_r(byte opcode)
{
	byte* r = GetByteRegister_Dst(opcode);
	byte result = AddBytes_Two(*r, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	*r = result;

	return 4;
}

ulong CPU::INC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = AddBytes_Two(value, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	m_MMU->WriteByte(m_HL, result);

	return 12;
}

ulong CPU::DEC_r(byte opcode)
{
	byte* r = GetByteRegister_Dst(opcode);
	byte result = SubtractBytes_Two(*r, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	*r = result;

	return 4;
}

ulong CPU::DEC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = SubtractBytes_Two(value, 1, /*affectedFlags =*/ ZeroFlagMask | SubtractFlagMask | HalfCarryFlagMask);
	m_MMU->WriteByte(m_HL, result);

	return 12;
}

/*
This instruction conditionally adjusts the accumulator for BCD (binary coded decimal) addition
and subtraction operations. For addition(ADD, ADC, INC) or subtraction
(SUB, SBC, DEC, NEC), the following table indicates the operation performed :

N   C  Value of      H  Value of     Hex no   C flag after
	   high nibble      low nibble   added    execution

0   0      0-9       0     0-9       00       0
0   0      0-8       0     A-F       06       0
0   0      0-9       1     0-3       06       0
0   0      A-F       0     0-9       60       1
0   0      9-F       0     A-F       66       1
0   0      A-F       1     0-3       66       1
0   1      0-2       0     0-9       60       1
0   1      0-2       0     A-F       66       1
0   1      0-3       1     0-3       66       1
1   0      0-9       0     0-9       00       0
1   0      0-8       1     6-F       FA       0
1   1      7-F       0     0-9       A0       1
1   1      6-F       1     6-F       9A       1

Flags:
	Z : Set if ACC is Zero after operation, clear otherwise.
	N : Unaffected.
	H : Clear
	C : See instruction.

Example:
	If an addition operation is performed between 15 (BCD) and 27 (BCD), simple decimal
	arithmetic gives this result:

	  15
	+ 27
	----
	  42

	But when the binary representations are added in the Accumulator according to
	standard binary arithmetic:

	  0001 0101  15
	+ 0010 0111  27
	---------------
	  0011 1100  3C

	The sum is ambiguous. The DAA instruction adjusts this result so that correct
	BCD representation is obtained:

	  0011 1100  3C result
	+ 0000 0110  06 + error
	---------------
	  0100 0010  42 Correct BCD!
*/
ulong CPU::DAA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte err = 0x00; // error
	byte c = GetFlag(CarryFlag);
	byte h = GetFlag(HalfCarryFlag);

	if (!IsFlagSet(SubtractFlag))
	{
		if (c || (A >= 0x9A))
		{
			err += 0x60;
		}

		if (h || ((A & 0x0F) >= 0x0A))
		{
			err += 0x06;
		}
	}
	else
	{
		if (!c && h)
		{
			err += 0xFA;
		}
		else if (c && !h)
		{
			err += 0xA0;
		}
		else if (c && h)
		{
			err += 0x9A;
		}
	}

	byte result = A + err;
	SetHighByte(&m_AF, result);

	(result == 0x00) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(HalfCarryFlag);

	if ((A + err) > 0xFF)
	{
		SetFlag(CarryFlag);
	}

	return 4;
}

ulong CPU::CPL(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = A ^ 0xFF;
	SetHighByte(&m_AF, result);

	SetFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);

	return 4;
}

ulong CPU::ADD_HL_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort result = AddUShorts_Two(m_HL, *rr, /*affectedFlags =*/ SubtractFlagMask | HalfCarryFlagMask | CarryFlagMask);
	m_HL = result;

	return 8;
}

ulong CPU::INC_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort result = AddUShorts_Two(*rr, 1, /*affectedFlags =*/ 0x00);
	*rr = result;

	return 8;
}

ulong CPU::DEC_rr(byte opcode)
{
	ushort* rr = GetUShortRegister(opcode);
	ushort result = SubtractUShorts_Two(*rr, 1, /*affectedFlags =*/ 0x00);
	*rr = result;

	return 8;
}

ulong CPU::ADD_SP_dd(byte opcode)
{
	sbyte dd = (sbyte)ReadBytePCI();
	ushort result = (m_SP + dd);

	ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	((result & 0x0F) < (m_SP & 0x0F)) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((result & 0xFF) < (m_SP & 0xFF)) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	m_SP = result;

	return 16;
}

ulong CPU::LD_HL_SPdd(byte opcode)
{
	sbyte dd = (sbyte)ReadBytePCI();
	ushort result = (m_SP + dd);

	ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	((result & 0x0F) < (m_SP & 0x0F)) ? SetFlag(HalfCarryFlag) : ClearFlag(HalfCarryFlag);
	((result & 0xFF) < (m_SP & 0xFF)) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 12;
}

ulong CPU::RLCA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateLeft(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RLA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateLeftThroughCarry(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RRCA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateRight(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RRA(byte opcode)
{
	byte A = GetHighByte(m_AF);
	byte result = RotateRightThroughCarry(A, /*clearZeroFlag =*/ true);
	SetHighByte(&m_AF, result);

	return 4;
}

ulong CPU::RLC_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateLeft(*r);
	*r = result;

	return 8;
}

ulong CPU::RLC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateLeft(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::RL_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateLeftThroughCarry(*r);
	*r = result;

	return 8;
}

ulong CPU::RL_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateLeftThroughCarry(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::RRC_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateRight(*r);
	*r = result;

	return 8;
}

ulong CPU::RRC_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateRight(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::RR_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte result = RotateRightThroughCarry(*r);
	*r = result;

	return 8;
}

ulong CPU::RR_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte result = RotateRightThroughCarry(value);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::SLA_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GET_BIT(*r, 7);
	*r = (*r << 1);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SLA_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GET_BIT(value, 7);
	byte result = (value << 1);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::SRA_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GET_BIT(*r, 0);
	*r = (*r >> 1) | (*r & 0x80);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SRA_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GET_BIT(value, 0);
	byte result = (value >> 1) | (value & 0x80);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::SRL_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte cf = GET_BIT(*r, 0);
	*r = (*r >> 1);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SRL_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte cf = GET_BIT(value, 0);
	byte result = (value >> 1);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	(cf == 1) ? SetFlag(CarryFlag) : ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::SWAP_r(byte opcode)
{
	byte* r = GetByteRegister_Src(opcode);
	byte low = (*r & 0x0F);
	byte high = (*r & 0xF0);
	*r = (low << 4) | (high >> 4);

	(*r == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 8;
}

ulong CPU::SWAP_0xHL(byte opcode)
{
	byte value = m_MMU->ReadByte(m_HL);
	byte low = (value & 0x0F);
	byte high = (value & 0xF0);
	byte result = (low << 4) | (high >> 4);
	m_MMU->WriteByte(m_HL, result);

	(result == 0) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	ClearFlag(CarryFlag);

	return 16;
}

ulong CPU::BIT_n_r(byte opcode)
{
	byte bit = (opcode >> 3) & 0x07;
	byte* r = GetByteRegister_Src(opcode);

	!IS_BIT_SET(*r, bit) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);

	return 8;
}

ulong CPU::BIT_n_0xHL(byte opcode)
{
	byte bit = (opcode >> 3) & 0x07;
	byte value = m_MMU->ReadByte(m_HL);

	!IS_BIT_SET(value, bit) ? SetFlag(ZeroFlag) : ClearFlag(ZeroFlag);
	ClearFlag(SubtractFlag);
	SetFlag(HalfCarryFlag);

	return 16;
}

ulong CPU::SET_n_r(byte opcode)
{
	byte bit = (opcode >> 3) & 0x07;
	byte* r = GetByteRegister_Src(opcode);
	*r = SET_BIT(*r, bit);

	return 8;
}

ulong CPU::SET_n_0xHL(byte opcode)
{
	byte bit = (opcode >> 3) & 0x07;
	byte value = m_MMU->ReadByte(m_HL);
	byte result = SET_BIT(value, bit);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::RES_n_r(byte opcode)
{
	byte bit = (opcode >> 3) & 0x07;
	byte* r = GetByteRegister_Src(opcode);
	*r = CLEAR_BIT(*r, bit);

	return 8;
}

ulong CPU::RES_n_0xHL(byte opcode)
{
	byte bit = (opcode >> 3) & 0x07;
	byte value = m_MMU->ReadByte(m_HL);
	byte result = CLEAR_BIT(value, bit);
	m_MMU->WriteByte(m_HL, result);

	return 16;
}

ulong CPU::CCF(byte opcode)
{
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	IsFlagSet(CarryFlag) ? ClearFlag(CarryFlag) : SetFlag(CarryFlag);

	return 4;
}

ulong CPU::SCF(byte opcode)
{
	ClearFlag(SubtractFlag);
	ClearFlag(HalfCarryFlag);
	SetFlag(CarryFlag);

	return 4;
}

ulong CPU::NOP(byte opcode)
{
	return 4;
}

ulong CPU::HALT(byte opcode)
{
	m_isHalted = true;

	return 4;
}

ulong CPU::STOP(byte opcode)
{
	return NOP(opcode);
}

ulong CPU::DI(byte opcode)
{
	m_IME = 0;

	return 4;
}

ulong CPU::EI(byte opcode)
{
	m_IME = 1;

	return 4;
}

ulong CPU::JP_nn(byte opcode)
{
	ushort nn = ReadUShortPCI();
	m_PC = nn;

	return 16;
}

ulong CPU::JP_HL(byte opcode)
{
	m_PC = m_HL;

	return 4;
}

ulong CPU::JP_cc_nn(byte opcode)
{
	return OpcodeCondition(opcode) ? JP_nn(opcode) : 12;
}

ulong CPU::JR_dd(byte opcode)
{
	sbyte dd = (sbyte)ReadBytePCI();
	m_PC += dd;

	return 12;
}

ulong CPU::JR_cc_dd(byte opcode)
{
	return OpcodeCondition(opcode) ? JR_dd(opcode) : 8;
}

ulong CPU::CALL_nn(byte opcode)
{
	ushort nn = ReadUShortPCI();
	PushUShortToStack(m_PC);
	m_PC = nn;

	return 24;
}

ulong CPU::CALL_cc_nn(byte opcode)
{
	return OpcodeCondition(opcode) ? CALL_nn(opcode) : 12;
}

ulong CPU::RET(byte opcode)
{
	m_PC = PopUShortFromStack();

	return 16;
}

ulong CPU::RET_cc(byte opcode)
{
	return OpcodeCondition(opcode) ? (RET(opcode) + 4) : 8;
}

ulong CPU::RETI(byte opcode)
{
	m_IME = 1;
	m_PC = PopUShortFromStack();

	return 16;
}

ulong CPU::RST_n(byte opcode)
{
	// ##nnn###
	// 000 - 0x00
	// 001 - 0x08
	// 010 - 0x10
	// 011 - 0x18
	// 100 - 0x20
	// 101 - 0x28
	// 110 - 0x30
	// 111 - 0x38
	PushUShortToStack(m_PC);
	byte n = ((opcode >> 3) & 0x07);
	m_PC = (ushort)(n);

	return 16;
}
