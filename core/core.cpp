
#include "core.h"

#include "core/tables/parsers/alldealstableparser.h"
#include "core/tables/parsers/currentparametertableparser.h"

Core::Core(const boost::program_options::variables_map& config) :
	m_ddeServer(std::make_shared<DataImportServer>(config["dde-server-name"].as<std::string>(),
			config["dde-topic"].as<std::string>())),
	m_registry(std::make_shared<TableParserFactoryRegistry>()),
	m_io(cppio::createLineManager()),
	m_quotesourceServer(std::make_shared<goldmine::QuoteSource>(m_io, config["quotesource-endpoint"].as<std::string>())),
	m_run(false)
{
	m_registry->registerFactory("current_parameters", std::unique_ptr<TableParserFactory>(new CurrentParameterTableParserFactory));
	m_registry->registerFactory("all_deals", std::unique_ptr<TableParserFactory>(new AllDealsTableParserFactory));
}

Core::~Core()
{
}

void Core::run()
{
	m_run = true;
	while(m_run)
	{

	}
}
