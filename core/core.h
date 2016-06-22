
#ifndef CORE_H
#define CORE_H

#include "tables/tableparserfactoryregistry.h"
#include "tables/datasink.h"

#include "dataimportserver.h"

#include "cppio/iolinemanager.h"

#include "quotesource/quotesource.h"

#include <boost/program_options.hpp>

#include <atomic>
#include <memory>

class Core
{
public:
	Core(const boost::program_options::variable_map& config);
	virtual ~Core();

	void run();

private:
	DataImportServer::Ptr m_ddeServer;
	TableParserFactoryRegistry::Ptr m_registry;
	std::shared_ptr<cppio::IoLineManager> m_io;
	std::shared_ptr<goldmine::QuoteSource> m_quotesourceServer;
	std::atomic<bool> m_run;
};

#endif /* ifndef CORE_H */
