
#include "tableconstructor.h"

#include "log.h"
#include "exceptions.h"

#include "json.h"

TableConstructor::TableConstructor(const TableParserFactoryRegistry::Ptr& registry,
								const DataImportServer::Ptr& importServer)
{

}

TableConstructor::~TableConstructor()
{

}

void TableConstructor::readConfig(std::istream& stream)
{
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(stream, root))
	{
        BOOST_THROW_EXCEPTION(ParameterError() << errinfo_str("Unable to parse config: " + reader.getFormattedErrorMessages()));
	}

	for(const auto& cfg : root)
	{
		auto type = cfg["type"].asString();
		auto topic = cfg["topic"].asString();

		auto parser = m_registry->create(type, topic, m_importServer);
		if(!parser)
		{
			LOG(warning) << "Unable to create parser: " << type << " for topic: " << topic;
			continue;
		}
		parser->parseConfig(cfg);
		m_importServer->registerTableParser(parser);
	}
}
