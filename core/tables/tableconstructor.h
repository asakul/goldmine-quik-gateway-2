
#ifndef TABLECONSTRUCTOR_H
#define TABLECONSTRUCTOR_H

#include "tableparserfactoryregistry.h"

class TableConstructor
{
public:
	TableConstructor(const TableParserFactoryRegistry::Ptr& registry,
			const DataImportServer::Ptr& importServer);
	virtual ~TableConstructor();

private:
	TableParserFactoryRegistry::Ptr m_registry;
	DataImportServer::Ptr m_importServer;
};

#endif /* ifndef TABLECONSTRUCTOR_H */
