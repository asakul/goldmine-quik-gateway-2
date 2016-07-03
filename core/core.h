
#ifndef CORE_H
#define CORE_H

#include "tables/tableparserfactoryregistry.h"
#include "tables/datasink.h"

#include "dataimportserver.h"

#include "cppio/iolinemanager.h"

#include "quotesource/quotesource.h"
#include "broker/brokerserver.h"
#include "quotetable.h"

#include <boost/program_options.hpp>
#include <boost/program_options/variables_map.hpp>

#include <atomic>
#include <memory>

class Core : public std::enable_shared_from_this<Core>, public DataSink
{
public:
	Core(const boost::program_options::variables_map& config);
	virtual ~Core();

	void run();

	virtual void incomingTick(const std::string& ticker, const goldmine::Tick& tick) override;
private:
	DataImportServer::Ptr m_ddeServer;
	TableParserFactoryRegistry::Ptr m_registry;
	std::shared_ptr<cppio::IoLineManager> m_io;
	std::shared_ptr<goldmine::QuoteSource> m_quotesourceServer;
	std::shared_ptr<goldmine::BrokerServer> m_brokerServer;
	std::atomic<bool> m_run;
	std::string m_tablesConfig;
	QuoteTable::Ptr m_quoteTable;
};

#endif /* ifndef CORE_H */
