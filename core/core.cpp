
#include "core.h"

#include "core/tables/parsers/alldealstableparser.h"
#include "core/tables/parsers/currentparametertableparser.h"
#include "tables/tableconstructor.h"
#include "broker/paperbroker.h"
#include "broker/quikbroker.h"

#include "ui/mainwindow.h"

#include "log.h"
#include "exceptions.h"

#include <fstream>
#include <boost/optional.hpp>

Core::Core(const boost::program_options::variables_map& config) :
	m_ddeServer(std::make_shared<DataImportServer>(config["dde-server-name"].as<std::string>(),
			config["dde-topic"].as<std::string>())),
	m_registry(std::make_shared<TableParserFactoryRegistry>()),
	m_io(cppio::createLineManager()),
	m_quotesourceServer(std::make_shared<goldmine::QuoteSource>(m_io, config["quotesource-endpoint"].as<std::string>())),
	m_brokerServer(std::make_shared<goldmine::BrokerServer>(m_io, config["brokerserver-endpoint"].as<std::string>())),
	m_run(false),
	m_quoteTable(std::make_shared<QuoteTable>())
{
	m_registry->registerFactory("current_parameters", std::unique_ptr<TableParserFactory>(new CurrentParameterTableParserFactory));
	m_registry->registerFactory("all_deals", std::unique_ptr<TableParserFactory>(new AllDealsTableParserFactory));
	m_tablesConfig = config["tables-file"].as<std::string>();

	std::list<std::string> accounts;
	accounts.push_back(config["quik.account"].as<std::string>());
	m_brokerServer->registerBroker(std::make_shared<QuikBroker>(config["quik.dll-path"].as<std::string>(),
			config["quik.exe-path"].as<std::string>(), accounts));

	try
	{
		auto statsEndpoint = config["stats-endpoint"].as<std::string>();
		m_brokerServer->setTradeSink(statsEndpoint);
	}
	catch(const boost::bad_any_cast& e)
	{
		// Ignore
	}

	m_brokerServer->registerBroker(std::make_shared<PaperBroker>(100000000., m_quoteTable));
}

Core::~Core()
{
}

void Core::run()
{
	TableConstructor constructor(m_registry, m_ddeServer, shared_from_this());

	std::fstream tablesConfig(m_tablesConfig, std::ios_base::in);
	if(!tablesConfig.good())
		BOOST_THROW_EXCEPTION(ParameterError() << errinfo_str("Unable to open file: " + m_tablesConfig));
	constructor.readConfig(tablesConfig);

	m_quotesourceServer->start();
	m_brokerServer->start();
	MainWindow wnd;
	wnd.show();
	LOG(info) << "Running main loop";
	auto rc = Fl::run();
}

void Core::incomingTick(const std::string& ticker, const goldmine::Tick& tick)
{
	{
		boost::unique_lock<boost::mutex> lock(m_mutex);
		m_quotesourceServer->incomingTick(ticker, tick);
	}
	m_quoteTable->updateQuote(ticker, tick);
}

