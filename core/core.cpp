
#include "core.h"

Core::Core(const boost::program_options::variable_map& config) :
	m_ddeServer(config["dde-server-name"].as<std::string>(),
			config["dde-topic"].as<std::string>()),
	m_registry(std::make_shared<TableParserFactoryRegistry>()),
	m_io(cppio::createLineManager()),
	m_quotesourceServer(std::make_shared<goldmine::QuoteSource>(m_io, config["quotesource-endpoint"].as<std::string>())),
	m_run(false)
{
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
