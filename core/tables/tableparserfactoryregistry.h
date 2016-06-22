/*
 * tableparserfactoryregistry.h
 */

#ifndef TABLES_TABLEPARSERFACTORYREGISTRY_H_
#define TABLES_TABLEPARSERFACTORYREGISTRY_H_

#include <memory>
#include <functional>
#include "tableparser.h"
#include "datasink.h"

class TableParserFactory
{
public:
	virtual ~TableParserFactory() = 0;

	virtual TableParser::Ptr create(const std::string& topic, const DataSink::Ptr& datasink) = 0;
};

inline TableParserFactory::~TableParserFactory() {}

class TableParserFactoryRegistry
{
public:
	typedef std::shared_ptr<TableParserFactoryRegistry> Ptr;

	TableParserFactoryRegistry();
	virtual ~TableParserFactoryRegistry();

	void registerFactory(const std::string& type, std::unique_ptr<TableParserFactory> f);

	TableParser::Ptr create(const std::string& type, const std::string& topic, const DataSink::Ptr& datasink);

private:
	std::map<std::string, std::unique_ptr<TableParserFactory>> m_factories;
};

#endif /* TABLES_TABLEPARSERFACTORYREGISTRY_H_ */
