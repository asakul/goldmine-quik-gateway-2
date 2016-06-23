
#ifndef TABLECONSTRUCTOR_H
#define TABLECONSTRUCTOR_H

#include "tableparserfactoryregistry.h"
#include "core/dataimportserver.h"

#include <istream>

class TableConstructor
{
public:
	TableConstructor(const TableParserFactoryRegistry::Ptr& registry,
			const DataImportServer::Ptr& importServer,
			const DataSink::Ptr& datasink);
	virtual ~TableConstructor();

	void readConfig(std::istream& stream);

private:
	TableParserFactoryRegistry::Ptr m_registry;
	DataImportServer::Ptr m_importServer;
	DataSink::Ptr m_datasink;
};

#endif /* ifndef TABLECONSTRUCTOR_H */
