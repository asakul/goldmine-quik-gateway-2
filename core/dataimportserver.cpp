/*
 * dataimportserver.cpp
 */

#include "dataimportserver.h"
#include "log.h"
#include "xl/xlparser.h"

#include "exceptions.h"
#include "log.h"

#include <boost/scope_exit.hpp>
#include <iomanip>

using namespace boost;

static DataImportServer* gs_server;

static logger_t gs_logger(boost::log::keywords::channel = "dde");

HDDEDATA theDdeCallback(UINT type, UINT fmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData, ULONG_PTR dwData1, ULONG_PTR dwData2)
{
	return gs_server->ddeCallback(type, fmt, hConv, hsz1, hsz2, hData, dwData1, dwData2);
}

DataImportServer::DataImportServer(const std::string& serverName, const std::string& topicName) : m_appName(0),
	m_topicName(0),
	m_instanceId(0)
{
	assert(!gs_server);
	gs_server = this;
	if(DdeInitialize(&m_instanceId, (PFNCALLBACK)theDdeCallback, APPCLASS_STANDARD, 0))
		BOOST_THROW_EXCEPTION(ExternalApiError() << errinfo_str("Unable to initialize DDE server"));

	m_appName = DdeCreateStringHandle(m_instanceId, serverName.c_str(), 0);
	if(!m_appName)
		BOOST_THROW_EXCEPTION(ExternalApiError() << errinfo_str("Unable to create string handle"));
	m_topicName = DdeCreateStringHandle(m_instanceId, topicName.c_str(), 0);
	if(!m_topicName)
		BOOST_THROW_EXCEPTION(ExternalApiError() << errinfo_str("Unable to create string handle"));

	if(!DdeNameService(m_instanceId, m_appName, NULL, DNS_REGISTER))
		BOOST_THROW_EXCEPTION(ExternalApiError() << errinfo_str("Unable to register DDE server"));
}

DataImportServer::~DataImportServer()
{
}

void DataImportServer::registerTableParser(const TableParser::Ptr& parser)
{
	if(std::find(m_tableParsers.begin(), m_tableParsers.end(), parser) == m_tableParsers.end())
		m_tableParsers.push_back(parser);
}

HDDEDATA DataImportServer::ddeCallback(UINT type, UINT fmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData, ULONG_PTR dwData1, ULONG_PTR dwData2)
{
	switch(type)
	{
	case XTYP_CONNECT:
		{
			char topicBuf[256];
			char appBuf[256];
			DdeQueryString(m_instanceId, hsz1, topicBuf, 256, CP_WINANSI);
			DdeQueryString(m_instanceId, hsz2, appBuf, 256, CP_WINANSI);
			LOG_WITH(gs_logger, info) << "Client connect: " << appBuf << " : " << topicBuf;
			if(!DdeCmpStringHandles(hsz2, m_appName))
				return (HDDEDATA)TRUE;
			else
				return (HDDEDATA)FALSE;
		}
		break;
	case XTYP_POKE:
		{
			char topicBuf[256];
			DdeQueryString(m_instanceId, hsz1, topicBuf, 256, CP_WINANSI);
			std::string topic(topicBuf);

			parseIncomingData(topic, hData);

			return (HDDEDATA)DDE_FACK;
		}

	default:
		return NULL;
	}
}

bool DataImportServer::parseIncomingData(const std::string& topic, HDDEDATA hData)
{
	DWORD dataSize = 0;
	BYTE* data = DdeAccessData(hData, &dataSize);
	if(!data)
		return false;

	BOOST_SCOPE_EXIT(hData)
	{
		DdeUnaccessData(hData);
	} BOOST_SCOPE_EXIT_END;

	try
	{
		XlParser parser;
		parser.parse(data, dataSize);

		auto table = parser.getParsedTable();

		for(const auto& tp : m_tableParsers)
		{
			if(tp->acceptsTopic(topic))
				tp->incomingTable(table);
		}
	}
	catch(const std::exception& e)
	{
		LOG_WITH(gs_logger, warning) << "Unable to parse incoming XlTable: " << e.what();
	}

	return true;
}

void DataImportServer::incomingTick(const std::string& ticker, const goldmine::Tick& tick)
{

}
