
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
template<typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator << (std::basic_ostream<CharT, TraitsT>& strm, severity_level lvl)
{
	static const char* const str[] =
	{
		"trace",
		"debug",
		"info ",
		"warn ",
		"error",
		"fatal"
	};

	if (static_cast<std::size_t>(lvl) < (sizeof(str) / sizeof(*str)))
		strm << str[(int)lvl];
	else
		strm << static_cast<int>(lvl);
	return strm;
}

using logger_t = boost::log::sources::severity_channel_logger_mt <severity_level>;

BOOST_LOG_GLOBAL_LOGGER(global_logger, boost::log::sources::severity_channel_logger_mt<severity_level>)

#define LOG(x) BOOST_LOG_SEV(global_logger::get(), severity_level::x)
#define LOG_CHANNEL(chan) BOOST_LOG_CHANNEL(global_logger::get(), chan)
#define LOG_CHANNEL_SEV(chan, x) BOOST_LOG_SEV_CHANNEL(global_logger::get(), chan, severity_level::x)

#define LOG_WITH(lg, x) BOOST_LOG_SEV(lg, severity_level::x)

void init_log(bool debug);

#endif /* ifndef LOG_H */
