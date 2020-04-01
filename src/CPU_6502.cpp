#include "CPU_6502.h"


CPU_6502::CPU_6502() {

}

CPU_6502::CPU_6502(const char* debugFilePath) {
	m_debugFile.open(debugFilePath, std::ofstream::out);
	
	if (!m_debugFile.is_open()) {
		fmt::print("Failed to open cpu.log file!");
	}
}

CPU_6502::~CPU_6502() {
	m_debugFile.flush();
	m_debugFile.close();
}

struct CPU_6502::CpuInstruction
{
	std::string name;
	uint8_t(CPU_6502::* operation)(void) = nullptr;
	uint8_t(CPU_6502::* addressMode)(void) = nullptr;
	uint8_t cycles = 0;
};

void CPU_6502::connectBus(Bus<uint16_t, uint8_t>* bus) {
	m_bus = bus;
}

bool CPU_6502::isIMP() {
	return (lookup[opcode].addressMode == &CPU_6502::IMP);
}

uint8_t CPU_6502::fetchData() {
	if (!(lookup[opcode].addressMode == &CPU_6502::IMP))
		fetchedData = read(addressAbsolute);

	return fetchedData;
}

// CPU Reset Function
void CPU_6502::reset() {
	size_t totalCyclesPassed = 0;

	// Get address for start of execution
	// and set the Program Counter to it
	addressAbsolute = 0xFFFC;
	uint16_t lo = read(addressAbsolute);
	uint16_t hi = read(addressAbsolute + 1);
	PC = (hi << 8) | lo;

	// Reset registers to known state
	A = 0;	X = 0;	Y = 0;
	SP = 0xFD;
	PS.data = 0x24;

	// Reset helper variables
	addressAbsolute = 0x0000;
	addressRelative = 0x0000;
	fetchedData = 0x00;

	// Reset takes 7 cycles
	cycles = 7;
}

// Interrupt Request
void CPU_6502::irq() {
	if (PS.ID == 0) {
		push(highByte(PC));
		push(lowByte(PC));

		PS.BC = 1;
		PS.ID = 1;
		push(PS.data);

		// Read new program counter location from fixed address
		uint16_t lo = read(irqVectorLow);
		uint16_t hi = read(irqVectorHigh);
		PC = (hi << 8) | lo;

		// IRQ taks 7 cycles
		cycles = 7;
	}
}

// Non-Maskable Interrupt
void CPU_6502::nmi() {
	push(highByte(PC));
	push(lowByte(PC));

	PS.BC = 1;
	PS.ID = 1;
	push(PS.data);

	uint16_t lo = read(nmiVectorLow);
	uint16_t hi = read(nmiVectorHigh);
	PC = (hi << 8) | lo;

	// NMI taks 8 cycles
	cycles = 8;
}

// Runs every clock cycle
void CPU_6502::tick() {
	if (cycles == 0) {
		opcode = read(PC++);
		
		PS.XX = 1;
		PS.DM = 0;
		(this->lookup[opcode].name).copy(currentInstructionName, sizeof currentInstructionName);

		cycles = lookup[opcode].cycles;

		uint8_t additionalCycle1 = (this->*lookup[opcode].addressMode)();
		uint8_t additionalCycle2 = (this->*lookup[opcode].operation)();

		cycles += (additionalCycle1 & additionalCycle2);
	}

	totalCyclesPassed++;
	cycles--;
}

bool CPU_6502::isFinished() {
	return cycles == 0;
}

//	+-----------------------+
//	|	  Lookup Vector		|
//	+-----------------------+

using c = CPU_6502;

std::vector<CPU_6502::CpuInstruction> CPU_6502::lookup = 
{
	{ "BRK", &c::BRK, &c::IMM, 7 },{ "ORA", &c::ORA, &c::IZX, 6 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 3 },{ "ORA", &c::ORA, &c::ZP0, 3 },{ "ASL", &c::ASL, &c::ZP0, 5 },{ "???", &c::XXX, &c::IMP, 5 },{ "PHP", &c::PHP, &c::IMP, 3 },{ "ORA", &c::ORA, &c::IMM, 2 },{ "ASL", &c::ASL, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::NOP, &c::IMP, 4 },{ "ORA", &c::ORA, &c::ABS, 4 },{ "ASL", &c::ASL, &c::ABS, 6 },{ "???", &c::XXX, &c::IMP, 6 },
	{ "BPL", &c::BPL, &c::REL, 2 },{ "ORA", &c::ORA, &c::IZY, 5 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 4 },{ "ORA", &c::ORA, &c::ZPX, 4 },{ "ASL", &c::ASL, &c::ZPX, 6 },{ "???", &c::XXX, &c::IMP, 6 },{ "CLC", &c::CLC, &c::IMP, 2 },{ "ORA", &c::ORA, &c::ABY, 4 },{ "???", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 7 },{ "???", &c::NOP, &c::IMP, 4 },{ "ORA", &c::ORA, &c::ABX, 4 },{ "ASL", &c::ASL, &c::ABX, 7 },{ "???", &c::XXX, &c::IMP, 7 },
	{ "JSR", &c::JSR, &c::ABS, 6 },{ "AND", &c::AND, &c::IZX, 6 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "BIT", &c::BIT, &c::ZP0, 3 },{ "AND", &c::AND, &c::ZP0, 3 },{ "ROL", &c::ROL, &c::ZP0, 5 },{ "???", &c::XXX, &c::IMP, 5 },{ "PLP", &c::PLP, &c::IMP, 4 },{ "AND", &c::AND, &c::IMM, 2 },{ "ROL", &c::ROL, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 2 },{ "BIT", &c::BIT, &c::ABS, 4 },{ "AND", &c::AND, &c::ABS, 4 },{ "ROL", &c::ROL, &c::ABS, 6 },{ "???", &c::XXX, &c::IMP, 6 },
	{ "BMI", &c::BMI, &c::REL, 2 },{ "AND", &c::AND, &c::IZY, 5 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 4 },{ "AND", &c::AND, &c::ZPX, 4 },{ "ROL", &c::ROL, &c::ZPX, 6 },{ "???", &c::XXX, &c::IMP, 6 },{ "SEC", &c::SEC, &c::IMP, 2 },{ "AND", &c::AND, &c::ABY, 4 },{ "???", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 7 },{ "???", &c::NOP, &c::IMP, 4 },{ "AND", &c::AND, &c::ABX, 4 },{ "ROL", &c::ROL, &c::ABX, 7 },{ "???", &c::XXX, &c::IMP, 7 },
	{ "RTI", &c::RTI, &c::IMP, 6 },{ "EOR", &c::EOR, &c::IZX, 6 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 3 },{ "EOR", &c::EOR, &c::ZP0, 3 },{ "LSR", &c::LSR, &c::ZP0, 5 },{ "???", &c::XXX, &c::IMP, 5 },{ "PHA", &c::PHA, &c::IMP, 3 },{ "EOR", &c::EOR, &c::IMM, 2 },{ "LSR", &c::LSR, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 2 },{ "JMP", &c::JMP, &c::ABS, 3 },{ "EOR", &c::EOR, &c::ABS, 4 },{ "LSR", &c::LSR, &c::ABS, 6 },{ "???", &c::XXX, &c::IMP, 6 },
	{ "BVC", &c::BVC, &c::REL, 2 },{ "EOR", &c::EOR, &c::IZY, 5 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 4 },{ "EOR", &c::EOR, &c::ZPX, 4 },{ "LSR", &c::LSR, &c::ZPX, 6 },{ "???", &c::XXX, &c::IMP, 6 },{ "CLI", &c::CLI, &c::IMP, 2 },{ "EOR", &c::EOR, &c::ABY, 4 },{ "???", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 7 },{ "???", &c::NOP, &c::IMP, 4 },{ "EOR", &c::EOR, &c::ABX, 4 },{ "LSR", &c::LSR, &c::ABX, 7 },{ "???", &c::XXX, &c::IMP, 7 },
	{ "RTS", &c::RTS, &c::IMP, 6 },{ "ADC", &c::ADC, &c::IZX, 6 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 3 },{ "ADC", &c::ADC, &c::ZP0, 3 },{ "ROR", &c::ROR, &c::ZP0, 5 },{ "???", &c::XXX, &c::IMP, 5 },{ "PLA", &c::PLA, &c::IMP, 4 },{ "ADC", &c::ADC, &c::IMM, 2 },{ "ROR", &c::ROR, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 2 },{ "JMP", &c::JMP, &c::IND, 5 },{ "ADC", &c::ADC, &c::ABS, 4 },{ "ROR", &c::ROR, &c::ABS, 6 },{ "???", &c::XXX, &c::IMP, 6 },
	{ "BVS", &c::BVS, &c::REL, 2 },{ "ADC", &c::ADC, &c::IZY, 5 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 4 },{ "ADC", &c::ADC, &c::ZPX, 4 },{ "ROR", &c::ROR, &c::ZPX, 6 },{ "???", &c::XXX, &c::IMP, 6 },{ "SEI", &c::SEI, &c::IMP, 2 },{ "ADC", &c::ADC, &c::ABY, 4 },{ "???", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 7 },{ "???", &c::NOP, &c::IMP, 4 },{ "ADC", &c::ADC, &c::ABX, 4 },{ "ROR", &c::ROR, &c::ABX, 7 },{ "???", &c::XXX, &c::IMP, 7 },
	{ "???", &c::NOP, &c::IMP, 2 },{ "STA", &c::STA, &c::IZX, 6 },{ "???", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 6 },{ "STY", &c::STY, &c::ZP0, 3 },{ "STA", &c::STA, &c::ZP0, 3 },{ "STX", &c::STX, &c::ZP0, 3 },{ "???", &c::XXX, &c::IMP, 3 },{ "DEY", &c::DEY, &c::IMP, 2 },{ "???", &c::NOP, &c::IMP, 2 },{ "TXA", &c::TXA, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 2 },{ "STY", &c::STY, &c::ABS, 4 },{ "STA", &c::STA, &c::ABS, 4 },{ "STX", &c::STX, &c::ABS, 4 },{ "???", &c::XXX, &c::IMP, 4 },
	{ "BCC", &c::BCC, &c::REL, 2 },{ "STA", &c::STA, &c::IZY, 6 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 6 },{ "STY", &c::STY, &c::ZPX, 4 },{ "STA", &c::STA, &c::ZPX, 4 },{ "STX", &c::STX, &c::ZPY, 4 },{ "???", &c::XXX, &c::IMP, 4 },{ "TYA", &c::TYA, &c::IMP, 2 },{ "STA", &c::STA, &c::ABY, 5 },{ "TXS", &c::TXS, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 5 },{ "???", &c::NOP, &c::IMP, 5 },{ "STA", &c::STA, &c::ABX, 5 },{ "???", &c::XXX, &c::IMP, 5 },{ "???", &c::XXX, &c::IMP, 5 },
	{ "LDY", &c::LDY, &c::IMM, 2 },{ "LDA", &c::LDA, &c::IZX, 6 },{ "LDX", &c::LDX, &c::IMM, 2 },{ "???", &c::XXX, &c::IMP, 6 },{ "LDY", &c::LDY, &c::ZP0, 3 },{ "LDA", &c::LDA, &c::ZP0, 3 },{ "LDX", &c::LDX, &c::ZP0, 3 },{ "???", &c::XXX, &c::IMP, 3 },{ "TAY", &c::TAY, &c::IMP, 2 },{ "LDA", &c::LDA, &c::IMM, 2 },{ "TAX", &c::TAX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 2 },{ "LDY", &c::LDY, &c::ABS, 4 },{ "LDA", &c::LDA, &c::ABS, 4 },{ "LDX", &c::LDX, &c::ABS, 4 },{ "???", &c::XXX, &c::IMP, 4 },
	{ "BCS", &c::BCS, &c::REL, 2 },{ "LDA", &c::LDA, &c::IZY, 5 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 5 },{ "LDY", &c::LDY, &c::ZPX, 4 },{ "LDA", &c::LDA, &c::ZPX, 4 },{ "LDX", &c::LDX, &c::ZPY, 4 },{ "???", &c::XXX, &c::IMP, 4 },{ "CLV", &c::CLV, &c::IMP, 2 },{ "LDA", &c::LDA, &c::ABY, 4 },{ "TSX", &c::TSX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 4 },{ "LDY", &c::LDY, &c::ABX, 4 },{ "LDA", &c::LDA, &c::ABX, 4 },{ "LDX", &c::LDX, &c::ABY, 4 },{ "???", &c::XXX, &c::IMP, 4 },
	{ "CPY", &c::CPY, &c::IMM, 2 },{ "CMP", &c::CMP, &c::IZX, 6 },{ "???", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "CPY", &c::CPY, &c::ZP0, 3 },{ "CMP", &c::CMP, &c::ZP0, 3 },{ "DEC", &c::DEC, &c::ZP0, 5 },{ "???", &c::XXX, &c::IMP, 5 },{ "INY", &c::INY, &c::IMP, 2 },{ "CMP", &c::CMP, &c::IMM, 2 },{ "DEX", &c::DEX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 2 },{ "CPY", &c::CPY, &c::ABS, 4 },{ "CMP", &c::CMP, &c::ABS, 4 },{ "DEC", &c::DEC, &c::ABS, 6 },{ "???", &c::XXX, &c::IMP, 6 },
	{ "BNE", &c::BNE, &c::REL, 2 },{ "CMP", &c::CMP, &c::IZY, 5 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 4 },{ "CMP", &c::CMP, &c::ZPX, 4 },{ "DEC", &c::DEC, &c::ZPX, 6 },{ "???", &c::XXX, &c::IMP, 6 },{ "CLD", &c::CLD, &c::IMP, 2 },{ "CMP", &c::CMP, &c::ABY, 4 },{ "NOP", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 7 },{ "???", &c::NOP, &c::IMP, 4 },{ "CMP", &c::CMP, &c::ABX, 4 },{ "DEC", &c::DEC, &c::ABX, 7 },{ "???", &c::XXX, &c::IMP, 7 },
	{ "CPX", &c::CPX, &c::IMM, 2 },{ "SBC", &c::SBC, &c::IZX, 6 },{ "???", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "CPX", &c::CPX, &c::ZP0, 3 },{ "SBC", &c::SBC, &c::ZP0, 3 },{ "INC", &c::INC, &c::ZP0, 5 },{ "???", &c::XXX, &c::IMP, 5 },{ "INX", &c::INX, &c::IMP, 2 },{ "SBC", &c::SBC, &c::IMM, 2 },{ "NOP", &c::NOP, &c::IMP, 2 },{ "???", &c::SBC, &c::IMP, 2 },{ "CPX", &c::CPX, &c::ABS, 4 },{ "SBC", &c::SBC, &c::ABS, 4 },{ "INC", &c::INC, &c::ABS, 6 },{ "???", &c::XXX, &c::IMP, 6 },
	{ "BEQ", &c::BEQ, &c::REL, 2 },{ "SBC", &c::SBC, &c::IZY, 5 },{ "???", &c::XXX, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 8 },{ "???", &c::NOP, &c::IMP, 4 },{ "SBC", &c::SBC, &c::ZPX, 4 },{ "INC", &c::INC, &c::ZPX, 6 },{ "???", &c::XXX, &c::IMP, 6 },{ "SED", &c::SED, &c::IMP, 2 },{ "SBC", &c::SBC, &c::ABY, 4 },{ "NOP", &c::NOP, &c::IMP, 2 },{ "???", &c::XXX, &c::IMP, 7 },{ "???", &c::NOP, &c::IMP, 4 },{ "SBC", &c::SBC, &c::ABX, 4 },{ "INC", &c::INC, &c::ABX, 7 },{ "???", &c::XXX, &c::IMP, 7 },
};

std::map<uint16_t, std::string> CPU_6502::disassemble(uint16_t nStart, uint16_t nStop)
{
	uint32_t addr = nStart;
	uint8_t value = 0x00, lo = 0x00, hi = 0x00;
	std::map<uint16_t, std::string> mapLines;
	uint16_t line_addr = 0;

	// A convenient utility to convert variables into
	// hex strings because "modern C++"'s method with 
	// streams is atrocious
	auto hex = [](uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	// Starting at the specified address we read an instruction
	// byte, which in turn yields information from the lookup table
	// as to how many additional bytes we need to read and what the
	// addressing mode is. I need this info to assemble human readable
	// syntax, which is different depending upon the addressing mode

	// As the instruction is decoded, a std::string is assembled
	// with the readable output
	while (addr <= (uint32_t)nStop)
	{
		line_addr = addr;

		// Prefix line with instruction address
		std::string sInst = "$" + hex(addr, 4) + ": ";

		// Read instruction, and get its readable name
		uint8_t opcode = read(addr); addr++;
		sInst += lookup[opcode].name + " ";

		// Get oprands from desired locations, and form the
		// instruction based upon its addressing mode. These
		// routines mimmick the actual fetch routine of the
		// 6502 in order to get accurate data as part of the
		// instruction
		if (lookup[opcode].addressMode == &CPU_6502::IMP)
		{
			sInst += " {IMP}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::IMM)
		{
			value = read(addr); addr++;
			sInst += "#$" + hex(value, 2) + " {IMM}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::ZP0)
		{
			lo = read(addr); addr++;
			hi = 0x00;
			sInst += "$" + hex(lo, 2) + " {ZP0}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::ZPX)
		{
			lo = read(addr); addr++;
			hi = 0x00;
			sInst += "$" + hex(lo, 2) + ", X {ZPX}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::ZPY)
		{
			lo = read(addr); addr++;
			hi = 0x00;
			sInst += "$" + hex(lo, 2) + ", Y {ZPY}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::IZX)
		{
			lo = read(addr); addr++;
			hi = 0x00;
			sInst += "($" + hex(lo, 2) + ", X) {IZX}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::IZY)
		{
			lo = read(addr); addr++;
			hi = 0x00;
			sInst += "($" + hex(lo, 2) + "), Y {IZY}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::ABS)
		{
			lo = read(addr); addr++;
			hi = read(addr); addr++;
			sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + " {ABS}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::ABX)
		{
			lo = read(addr); addr++;
			hi = read(addr); addr++;
			sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + ", X {ABX}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::ABY)
		{
			lo = read(addr); addr++;
			hi = read(addr); addr++;
			sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + ", Y {ABY}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::IND)
		{
			lo = read(addr); addr++;
			hi = read(addr); addr++;
			sInst += "($" + hex((uint16_t)(hi << 8) | lo, 4) + ") {IND}";
		}
		else if (lookup[opcode].addressMode == &CPU_6502::REL)
		{
			value = read(addr); addr++;
			sInst += "$" + hex(value, 2) + " [$" + hex(addr + value, 4) + "] {REL}";
		}

		// Add the formed string to a std::map, using the instruction's
		// address as the key. This makes it convenient to look for later
		// as the instructions are variable in length, so a straight up
		// incremental index is not sufficient.
		mapLines[line_addr] = sInst;
	}

	return mapLines;
}
