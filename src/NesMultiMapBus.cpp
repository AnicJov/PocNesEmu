#include "NesMultiMapBus.h"


NesMultiMapBus::~NesMultiMapBus() { }


void NesMultiMapBus::addSlave(IBusSlave<uint16_t, uint8_t>* slaveToAdd, uint16_t startAddressToAdd, uint16_t endAddressToAdd) {
	// Check if the end address goes above the address space
	if (((size_t)startAddressToAdd + slaveToAdd->size() - 1) > maxAddress) {
		throw std::overflow_error("Slave's end address overflows possible address space");
		return;
	}

	// Check every existing slave for overlap with new slave
	for (auto it = m_slaves.begin(); it != m_slaves.end(); ++it) {
		// Existing slave's addresses
		IBusSlave<uint16_t, uint8_t>* slave = it->first;
		uint16_t startAddress = it->second;
		uint16_t size = slave->size();
		uint16_t endAddress = startAddress + size - 1;

		// New slave's end address
		uint16_t endAddressToAdd = startAddressToAdd + slaveToAdd->size() - 1;

		// Check for overlap
		if (endAddressToAdd >= startAddress && endAddressToAdd < endAddress) {
			throw std::range_error("Slave addresses overlaping");
			return;
		}
		else if (startAddressToAdd >= startAddress && startAddressToAdd < endAddress) {
			throw std::range_error("Slave addresses overlaping");
			return;
		}
		else if (startAddress >= startAddressToAdd && endAddress < endAddressToAdd) {
			throw std::range_error("Slave addresses overlaping");
			return;
		}
	}

	// Add new slave
	fmt::printf("Added slave: $%04X-$%04X\n", (int)startAddressToAdd, startAddressToAdd + slaveToAdd->size() - 1);
	m_slaves.insert(std::make_pair(slaveToAdd, startAddressToAdd));
}


IBusSlave<uint16_t, uint8_t>* NesMultiMapBus::getSlaveWithAddress(uint16_t address) {
	// Check if address is in address space
	if (address < 0 || address > maxAddress) return nullptr;

	// Find slave that occupies the address and return it
	for (auto it = m_slaves.begin(); it != m_slaves.end(); ++it) {
		IBusSlave<uint16_t, uint8_t>* slave = it->first;
		uint16_t startAddress = it->second;

		if ((address >= startAddress) && (address <= (startAddress + slave->size() - 1))) {
			lastRetrievedStartAddress = startAddress;
			return slave;
		}
	}

	// If no slave has the address return nullptr
	return nullptr;
}


bool NesMultiMapBus::write(uint16_t address, uint8_t data, bool log) {
	// Write to appropriate slave
	auto slave = getSlaveWithAddress(address);
	if (slave == nullptr) {
		if (log)
			fmt::printf("No slaves match the address %04X. No write will happen.", (int)address);
		return false;
	}

	slave->write(address - lastRetrievedStartAddress, data);

	return true;
}


uint8_t NesMultiMapBus::read(uint16_t address, bool log, bool readOnly) {
	// Read from appropriate slave
	auto slave = getSlaveWithAddress(address);
	if (slave == nullptr) {
		if (log)
			fmt::printf("No slaves match the address %04X. Read will return -1 (0xFF).", (int)address);
		return -1;
	}

	return slave->read(address - lastRetrievedStartAddress);
}
