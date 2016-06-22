
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
	}
	else
	{
		init_log(false);
	}

	LOG(info) << "Goldmine-Quik-Gateway started";

	Core core(vm);

	core.run();

	return 0;
}

