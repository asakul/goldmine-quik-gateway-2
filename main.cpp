
#include "log.h"
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>

#include "core/core.h"

namespace po = boost::program_options;

int main(int argc, char** argv)
{
	po::options_description desc("Goldmine-Quik-Gateway");
	desc.add_options()
		("help", "Print help message")
		("debug", "Enables debug output")
		("tables-file", po::value<std::string>(), "Tables specification file")
		("dde-server-name", po::value<std::string>(), "DDE server name")
		("dde-topic", po::value<std::string>(), "DDE topic")
		("quotesource-endpoint", po::value<std::string>(), "Quotesource endpoint")
		("brokerserver-endpoint", po::value<std::string>(), "Brokerserver endpoint")
		("quik.account", po::value<std::string>(), "Account to use")
		("quik.dll-path", po::value<std::string>(), "Path to Trans2Quik.dll")
		("quik.exe-path", po::value<std::string>(), "Path to directory with QUIK executable")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::store(po::parse_config_file<char>("gateway.conf", desc, false), vm);
	po::notify(vm);

	if(vm.count("help"))
	{
		std::cout << desc << std::endl;
		return 1;
	}

	if(vm.count("debug"))
	{
		init_log(true);
		LOG(debug) << "Activated debug output";
	}
	else
	{
		init_log(false);
	}

	LOG(info) << "Goldmine-Quik-Gateway started";

	auto core = std::make_shared<Core>(vm);

	core->run();

	return 0;
}

