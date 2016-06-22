
#ifndef DATASINK_H
#define DATASINK_H

#include "goldmine/data.h"

#include <memory>

class DataSink
{
public:
	using Ptr = std::shared_ptr<DataSink>;

	virtual ~DataSink() = 0;

	virtual void incomingTick(const std::string& ticker, const goldmine::Tick& tick) = 0;
};

inline DataSink::~DataSink() {}

#endif /* ifndef DATASINK_H */
