#include <fstream>

#include "NesCore.h"



NesCore::NesCore(INesCpu* cpu, INesPpu* ppu, IRam<uint16_t, uint8_t>* ram,
	Bus<uint16_t, uint8_t>* cpuBus, Bus<uint16_t, uint8_t>* ppuBus) :
	m_cpu(cpu), m_ppu(ppu), m_ram(ram), m_cpuBus(cpuBus), m_ppuBus(ppuBus)
{
	// Connect CPU to its bus
	m_cpu->connectBus(m_cpuBus);

	// Add the system RAM to the bus
	m_cpuBus->addSlave(m_ram, 0x0000);
	m_cpuBus->addSlave(m_ram, 0x0800);
	m_cpuBus->addSlave(m_ram, 0x1000);
	m_cpuBus->addSlave(m_ram, 0x1800);

	// Add the PPU to the CPU Bus
	m_cpuBus->addSlave(m_ppu, 0x2000);

	// Connect PPU to its bus
	m_ppu->connectBus(m_ppuBus);

	// Add PPU's RAMs to its bus
	m_patternTable = new NesArrayRam(0x2000);
	m_nameTable = new NesArrayRam(0x1000);
	m_palletteRam = new NesArrayRam(0x20);
	m_ppuBus->addSlave(m_patternTable, 0x0000);
	m_ppuBus->addSlave(m_nameTable, 0x2000);

	// Add pallette RAM mirrored
	for (int i = 0; i < 8; i++) {
		m_ppuBus->addSlave(m_palletteRam, 0x3F00 + (i*0x20));
	}
}


NesCore::~NesCore() {
	m_cpu->~INesCpu();
	m_ram->~IRam();
	m_cpuBus->~Bus();
	m_ppuBus->~Bus();
	m_patternTable->~IRam();
	m_nameTable->~IRam();
	m_palletteRam->~IRam();
}


// Maybe move this into Bus.h?
void NesCore::dump_memory(Bus<uint16_t, uint8_t>* bus, size_t startAddress = 0, size_t endAddress = 0xFFFF) {
	std::ofstream memDumpFile(MEM_DUMP_FILE_PATH, std::ofstream::out);
	if (!memDumpFile.is_open()) {
		fmt::print("Failed to open memdump.log file!\n");
		return;
	}

	for (int i = 0; i <= 0xFFFF; i++) {
		if (i % 0x10 == 0) {
			fmt::fprintf(memDumpFile, "\n0x%04X: ", i);
		}
		fmt::fprintf(memDumpFile, "%02X ", bus->read(i, false, false));
	}
	memDumpFile.flush();
	memDumpFile.close();
	fmt::printf("Dumped memory to disk ($%04X-$%04X).\n", startAddress, endAddress);
}


void NesCore::runCPU_nCycles(size_t nCycles) {
	m_cpu->reset();
	do {
		m_cpu->tick();
	} while (m_cpu->getCyclesPassed() <= nCycles);
}


void NesCore::runCPU_nInstructions(size_t nInstructions) {
	m_cpu->reset();
	do {
		m_cpu->tick();
		if (m_cpu->isFinished())
			nInstructions--;
	} while (nInstructions > 0);
}


bool NesCore::loadCartridge(const char* filePath) {
	IRam<uint16_t, uint8_t>* mainRom;
	IRam<uint16_t, uint8_t>* otherRom;

	std::streampos size;
	uint8_t* memblock = nullptr;

	std::ifstream nesTestRom(filePath,
		std::ios::binary | std::ios::in | std::ios::ate);

	if (!nesTestRom.is_open()) {
		fmt::print("Couldn't open ROM file!\n");;

		return 1;
	}

	size = nesTestRom.tellg();
	memblock = new uint8_t[size];
	nesTestRom.seekg(0, std::ios::beg);
	nesTestRom.read((char*)memblock, size);
	const int romSize = nesTestRom.gcount();
	nesTestRom.close();

	fmt::print("ROM file read.\n");

	const uint16_t baseAddress = 0x8000;
	const uint16_t otherAddress = 0xC000;
	const uint16_t romOffset = 0x0010;

	mainRom = new NesArrayRam(0x4000);
	otherRom = new NesArrayRam(0x4000);
	m_cpuBus->addSlave(mainRom, baseAddress);
	m_cpuBus->addSlave(otherRom, otherAddress);

	for (int i = 0; i < 0x4000; i++) {
		m_cpuBus->write(baseAddress + i, memblock[romOffset + i]);
		m_cpuBus->write(otherAddress + i, memblock[romOffset + i]);
	}

	delete[] memblock;

	fmt::print("ROM loaded into memory.\n");
}


void NesCore::nesTest() {
	// Load ROM
	loadCartridge(NESTEST_FILE_PATH);

	// Write reset vector
	m_cpuBus->write(0xFFFC, 0x00);
	m_cpuBus->write(0xFFFD, 0xC0);

	// Run the CPU
	runCPU_nCycles(26554);		// NESTest runs for 26554 cycles

	// Memory dump
	fmt::print("\n");
	dump_memory(m_cpuBus);
	dump_memory(m_ppuBus);
}
