#pragma once

#include "IRam.h"


class NesArrayRam final : public IRam<uint16_t, uint8_t> {
public:
	NesArrayRam(uint16_t size) : m_size(size) {
		m_data = new uint8_t[m_size];

		for (int i = 0; i < m_size; i++)
			m_data[i] = 0;
	}

	~NesArrayRam() {
		delete[] m_data;
	}

	uint16_t inline const size() override {
		return m_size;
	}

	uint8_t read(uint16_t address) override {
		return m_data[address % m_size];
	}

	void write(uint16_t address, uint8_t data) override {
		m_data[address % m_size] = data;
	}

private:
	uint8_t* m_data;
	uint16_t m_size;

};
