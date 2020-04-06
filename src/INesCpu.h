#pragma once

#include <cstdint>

#include "IBusMaster.h"


class INesCpu : public IBusMaster<uint16_t, uint8_t> {
public:
	virtual void reset() = 0;
	virtual void tick() = 0;
	virtual bool isFinished() = 0;
	virtual const inline size_t getCyclesPassed() = 0;

	virtual ~INesCpu() {}

protected:
	virtual void irq() = 0;
	virtual void nmi() = 0;

};
