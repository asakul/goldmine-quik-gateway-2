/*
 * currentparametertableparser.cpp
 */

#include "currentparametertableparser.h"
#include "log.h"
#include <cstdlib>
#include <chrono>

enum ColumnId
{
	ClassCode = 0,
	Code,
	Bid,
	Ask,
	LastPrice,
	OpenInterest,
	TotalBid,
	TotalAsk,
	Volume,
	MaxId
};
static std::vector<std::string> gs_columnNames = {
		"CLASS_CODE",
		"CODE",
		"bid",
		"offer",
		"last",
		"numcontracts",
		"biddeptht",
		"offerdeptht",
		"voltoday" };

static int indexOf(const std::string& code)
{
	auto it = std::find(gs_columnNames.begin(), gs_columnNames.end(), code);
	if(it != gs_columnNames.end())
		return std::distance(gs_columnNames.begin(), it);
	return -1;
}

static std::map<std::string, goldmine::Datatype> gs_datatypeMap
{
	{ "price", goldmine::Datatype::Price },
	{ "open_interest", goldmine::Datatype::OpenInterest },
	{ "best_bid", goldmine::Datatype::BestBid },
	{ "best_offer", goldmine::Datatype::BestOffer },
	{ "depth", goldmine::Datatype::Depth },
	{ "total_supply", goldmine::Datatype::TotalSupply },
	{ "total_demand", goldmine::Datatype::TotalDemand }
};
static goldmine::Datatype deserializeDatatype(const std::string& str)
{
	return gs_datatypeMap[str];
}

CurrentParameterTableParser::CurrentParameterTableParser(const std::string& topic,
		const DataSink::Ptr& datasink) : m_topic(topic),
	m_datasink(datasink)
{
	LOG(trace) << "CurrentParameterTableParser: " << topic;
}

CurrentParameterTableParser::~CurrentParameterTableParser()
{
}

bool CurrentParameterTableParser::acceptsTopic(const std::string& topic)
{
	return topic == m_topic;
}

void CurrentParameterTableParser::incomingTable(const XlTable::Ptr& table)
{
	if(!schemaObtained())
		obtainSchema(table);

	try
	{
		for(int row = 0; row < table->height(); row++)
		{
			auto firstCell = table->get(row, 0);
			if(!boost::get<XlTable::XlEmpty>(&firstCell))
				parseRow(row, table);
		}
	}
	catch(const std::exception& e)
	{
		LOG(warning) << "Unable to parse incoming table, exception thrown: " << e.what();
	}
}

void CurrentParameterTableParser::parseConfig(const Json::Value& root)
{
	auto ignoreList = root["ignore"];
	if(!ignoreList.isNull())
	{
		auto it = ignoreList.begin();
		while(it != ignoreList.end())
		{
            auto ignoreItem = it->asString();
            auto slash = ignoreItem.find('/');
            if(slash == std::string::npos)
			{
				m_filters.push_back([&](const std::string& ticker, goldmine::Datatype type)
				{
					return ticker == ignoreItem;
				});
			}
			else
			{
				auto ignoreTicker = ignoreItem.substr(0, slash);
				auto datatype = deserializeDatatype(ignoreItem.substr(slash + 1));
				m_filters.push_back([&](const std::string& ticker, goldmine::Datatype type)
				{
					return (ticker == ignoreTicker) && (datatype == type);
				});
			}
		}
	}
}

bool CurrentParameterTableParser::schemaObtained() const
{
	return !m_schema.empty();
}

void CurrentParameterTableParser::obtainSchema(const XlTable::Ptr& table)
{
	m_schema.resize(MaxId, -1);
	for(int i = 0; i < table->width(); i++)
	{
		std::string header;

		try
		{
			header = boost::get<std::string>(table->get(0, i));
		}
		catch(const boost::bad_get& e)
		{
			// Skip silently
		}
		int index = indexOf(header);
		m_schema[index] = i;
	}
}

void CurrentParameterTableParser::parseRow(int row, const XlTable::Ptr& table)
{
	std::string code;
	try
	{
		auto contractClassCode = boost::get<std::string>(table->get(row, m_schema[ClassCode]));
		auto contractCode = boost::get<std::string>(table->get(row, m_schema[Code]));
		code = contractClassCode + "#" + contractCode;
	}
	catch(const boost::bad_get& e)
	{
		LOG(warning) << "Unable to parse contract code from table";
		return;
	}

	long volume = 1;
	try
	{
		auto cumulativeVolume = boost::get<double>(table->get(row, m_schema[Volume]));
		long lastVolume = m_volumes[code];
		if(cumulativeVolume < lastVolume)
		{
			lastVolume = 0;
		}
		if(lastVolume == 0)
		{
			lastVolume = cumulativeVolume;
			volume = 0;
		}
		else
		{
			volume = cumulativeVolume - lastVolume;
		}
		m_volumes[code] = cumulativeVolume;
	}
	catch(const boost::bad_get& e)
	{
		volume = 0;
	}

	auto currentTime = std::chrono::system_clock::now();

	goldmine::Tick tick;
	tick.timestamp = std::chrono::system_clock::to_time_t(currentTime);
	tick.useconds = 0;

	try
	{
		double delta = 1;
		auto lastPrice = boost::get<double>(table->get(row, m_schema[LastPrice]));
		try
		{
			auto bidPrice = boost::get<double>(table->get(row, m_schema[Bid]));
			auto askPrice = boost::get<double>(table->get(row, m_schema[Ask]));

			if(lastPrice == bidPrice)
			{
				delta = -1;
			}
			else if(lastPrice == askPrice)
			{
				delta = 1;
			}
			else if(lastPrice <= m_bids[code])
			{
				delta = -1;
			}
			else if(lastPrice >= m_asks[code])
			{
				delta = 1;
			}
			else if(lastPrice == m_prices[code])
			{
				// Make a random guess
				delta = (rand() % 2) == 0 ? 1 : -1;
			}
			else
			{
				delta = lastPrice - m_prices[code];
			}

			m_prices[code] = lastPrice;
			m_bids[code] = bidPrice;
			m_asks[code] = askPrice;

			tick.datatype = (int)goldmine::Datatype::BestBid;
			tick.value = bidPrice;
			tick.volume = 0;
			emitTick(code, tick);

			tick.datatype = (int)goldmine::Datatype::BestOffer;
			tick.value = askPrice;
			tick.volume = 0;
			emitTick(code, tick);

		}
		catch(const boost::bad_get& e)
		{
			// If we don't have best bid/ask data we should do nothing
		}

		if(std::abs(volume) > 0)
		{
			tick.datatype = (int)goldmine::Datatype::Price;
			tick.value = lastPrice;
			tick.volume = delta >= 0 ? volume : -volume;
			emitTick(code, tick);
		}
	}
	catch(const boost::bad_get& e)
	{
		// Whatever
	}


	try
	{
		auto openInterest = boost::get<double>(table->get(row, m_schema[OpenInterest]));
		tick.datatype = (int)goldmine::Datatype::OpenInterest;
		tick.value = openInterest;
		tick.volume = 0;
		emitTick(code, tick);
	}
	catch(const boost::bad_get& e)
	{
	}

	try
	{
		auto totalBid = boost::get<double>(table->get(row, m_schema[TotalBid]));

		tick.datatype = (int)goldmine::Datatype::TotalDemand;
		tick.value = totalBid;
		tick.volume = 0;
		emitTick(code, tick);
	}
	catch(const boost::bad_get& e)
	{
	}

	try
	{
		auto totalAsk = boost::get<double>(table->get(row, m_schema[TotalAsk]));
		tick.datatype = (int)goldmine::Datatype::TotalSupply;
		tick.value = totalAsk;
		tick.volume = 0;
		emitTick(code, tick);
	}
	catch(const boost::bad_get& e)
	{
	}
}

void CurrentParameterTableParser::emitTick(const std::string& ticker, goldmine::Tick& tick)
{
	for(const auto& filter : m_filters)
	{
		if(filter(ticker, (goldmine::Datatype)tick.datatype))
			return;
	}
	m_datasink->incomingTick(ticker, tick);
}

TableParser::Ptr CurrentParameterTableParserFactory::create(const std::string& topic, const DataSink::Ptr& datasink)
{
	return std::make_shared<CurrentParameterTableParser>(topic, datasink);
}

