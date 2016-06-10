
#include "log.h"
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>

namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

BOOST_LOG_GLOBAL_LOGGER_INIT(global_logger, boost::log::sources::severity_channel_logger<severity_level>)
{
	boost::log::sources::severity_channel_logger_mt<severity_level> lg(keywords::channel = "global");
	return lg;
}

void init_log(bool debug)
{
	boost::log::add_common_attributes();
	boost::log::add_file_log(
			boost::log::keywords::file_name = "goldmine-quik-gateway_%Y%m%d.log",
			boost::log::keywords::open_mode = (std::ios::out | std::ios::app),
			boost::log::keywords::format = expr::stream
				<< expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
				<< " >" << expr::attr<severity_level>("Severity") << "<"
				<< " [" << expr::attr<std::string>("Channel") << "] " << expr::smessage); //"[%TimeStamp%][%Channel%] <%Severity%>: %Message%");

	boost::log::core::get()->set_filter([=](const boost::log::attribute_value_set& attr_set)
			{
			return attr_set["Severity"].extract<severity_level>() >= severity_level::info;
			});
}

