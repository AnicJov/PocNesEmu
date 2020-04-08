#include <fstream>

#include "fmt/printf.h"

#include "NesCartridge.h"


NesCartridge::NesCartridge(const char* romFilePath) {
	std::ifstream romFile;
	romFile.open(romFilePath, std::ifstream::binary);

	if (!romFile.is_open()) {
		fmt::print("Failed to open ROM file!");
		return;
	}

	// Read iNES file header
	romFile.read((char*)&m_header, sizeof(romHeader));

	// Read training information, ignored for now
	// TODO: Read it
	if (m_header.mapper1 & 0x04)
		romFile.seekg(512, std::ios_base::cur);

	// Set mapper and mirroring information
	m_mapperID = ((m_header.mapper2 >> 4) << 4 | (m_header.mapper1 >> 4));
	mirrorMode = (m_header.mapper1 & 0x01) ? VERTICAL : HORIZONTAL;

	// Get type of iNES file
	// TODO: Do this instead of hard coding 1
	m_fileType = 1;

	// TODO: Implement more file type handlers
	switch (m_fileType) {
	case 0:
		fmt::print("iNES file type 0 not implemented!");
		break;
	case 1:
		// Read PRG memory
		m_PRGBanks = m_header.prg_rom_chunks;
		m_PRGMemorySize = (uint16_t)m_PRGBanks * 16384;
		m_PRGMemory = new uint8_t[m_PRGMemorySize];
		romFile.read((char*)m_PRGMemory, m_PRGMemorySize);

		// Read CHR memory
		m_CHRBanks = m_header.chr_rom_chunks;
		m_CHRMemorySize = (uint16_t)m_CHRBanks * 8192;
		m_CHRMemory = new uint8_t[m_CHRMemorySize];
		romFile.read((char*)m_CHRMemory, m_CHRMemorySize);
		break;
	case 2:
		fmt::print("iNES file type 2 not implemented!");
		break;
	default:
		fmt::print("Unsuported iNES file type!");
		break;
	}

	// Load appropriate mapper
	switch (m_mapperID) {
	case 0:
		m_mapper = std::make_shared<Mapper_000>(m_PRGBanks, m_CHRBanks);
		break;
	default:
		fmt::print("Unsuported mapper type!");
		break;
	}


	romFile.close();
}


NesCartridge::~NesCartridge() {
	if (m_PRGMemory != nullptr)
		delete[] m_PRGMemory;

	if (m_CHRMemory != nullptr)
		delete[] m_CHRMemory;
}


uint8_t NesCartridge::read(uint16_t address) {
	// PPU Read
	if (address >= 0x0000 && address <= 0x1FFF)
		return m_CHRMemory[m_mapper->mapRead(address)];
	
	// CPU Read
	if (address >= 0x8000 && address <= 0xFFFF)
		return m_PRGMemory[m_mapper->mapRead(address)];
}


void NesCartridge::write(uint16_t address, uint8_t data) {
	// PPU Write
	if (address >= 0x0000 && address <= 0x1FFF)
		m_PRGMemory[m_mapper->mapWrite(address)] = data;

	// CPU Write
	if (address >= 0x8000 && address <= 0xFFFF)
		m_CHRMemory[m_mapper->mapWrite(address)] = data;
}


// TODO: Fix this to somehow return the size?
inline const uint16_t NesCartridge::size() {
	return 0;
}

