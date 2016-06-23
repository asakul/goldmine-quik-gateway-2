
#include "tableconstructor.h"

#include "log.h"
#include "exceptions.h"

#include "json.h"

TableConstructor::TableConstructor(const TableParserFactoryRegistry::Ptr& registry,
								const DataImportServer::Ptr& importServer,
								const DataSink::Ptr& datasink) : m_registry(registry),
								m_importServer(importServer),
								m_datasink(datasink)
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
		LOG(debug) << "Trying to create parser: " << type;

		auto parser = m_registry->create(type, topic, m_datasink);
		if(!parser)
		{
			LOG(warning) << "Unable to create parser: " << type << " for topic: " << topic;
			continue;
		}
		parser->parseConfig(cfg);
		m_importServer->registerTableParser(parser);
	}
}
