
#ifndef LOG_H
#define LOG_H

#include <boost/log/trivial.hpp>
#include <boost/log/sources/channel_feature.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

enum class severity_level
{
	trace,
	debug,
	info,
	warning,
	error,
	fatal
};

BOOST_LOG_GLOBAL_LOGGER(global_logger, boost::log::sources::severity_channel_logger_mt<severity_level>)

void init_log(bool debug);

#endif /* ifndef LOG_H */
