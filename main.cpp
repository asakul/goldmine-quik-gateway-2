
#include "log.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
	po::options_description desc("Goldmine-Quik-Gateway");
	desc.add_options()
		("help", "Print help message")
		("debug", "Enables debug output")
		("tables-file", po::value<std::string>(), "Tables specification file");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
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

	LOG(severity_level::info) << "Goldmine-Quik-Gateway started";

	return 0;
}

