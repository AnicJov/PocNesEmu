#pragma once

#include <limits>

#include "IBusSlave.h"


template <typename addressWidth, typename dataWidth>
class IBus {
public:
	virtual void addSlave(std::shared_ptr<IBusSlave<addressWidth, dataWidth>> slave,
		addressWidth startAddress, addressWidth endAddress = 0) = 0;

	virtual std::shared_ptr<IBusSlave<addressWidth, dataWidth>>
		getSlaveWithAddress(addressWidth address) = 0;

	virtual bool write(addressWidth address, dataWidth data, bool log = false) = 0;
	virtual dataWidth read(addressWidth address,
		bool log = false, bool readOnly = false) = 0;

protected:
	static const size_t maxAddress = std::numeric_limits<addressWidth>::max();
};
