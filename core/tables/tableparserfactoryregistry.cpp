/*
 * tableparserfactoryregistry.cpp
 */

#include "tableparserfactoryregistry.h"

TableParserFactoryRegistry::TableParserFactoryRegistry()
{
}

TableParserFactoryRegistry::~TableParserFactoryRegistry()
{
}

void TableParserFactoryRegistry::registerFactory(const std::string& type, std::unique_ptr<TableParserFactory> f)
{
	m_factories.insert(std::make_pair(type, std::move(f)));
}

TableParser::Ptr TableParserFactoryRegistry::create(const std::string& type, const std::string& topic, const DataSink::Ptr& datasink)
{
	auto it = m_factories.find(type);
	if(it == m_factories.end())
		throw std::runtime_error("Unable to create parser with type: " + type);

	return it->second->create(topic, datasink);
}

