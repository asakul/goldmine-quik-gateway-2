#include "quikbroker.h"

#include "exceptions.h"
#include "log.h"

#include <atomic>
#include <boost/date_time/posix_time/posix_time.hpp>

static std::atomic_int gs_id;
QuikBroker* QuikBroker::m_instance;
std::unique_ptr<Trans2QuikApi> QuikBroker::m_quik;
static logger_t gs_logger(boost::log::keywords::channel = "quik");

using namespace goldmine;

time_t mkgmtime(const struct tm *tm)
{
	// Month-to-day offset for non-leap-years.
	static const int month_day[12] =
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

	// Most of the calculation is easy; leap years are the main difficulty.
	int month = tm->tm_mon % 12;
	int year = tm->tm_year + tm->tm_mon / 12;
	if (month < 0) {   // Negative values % 12 are still negative.
		month += 12;
		--year;
	}

	// This is the number of Februaries since 1900.
	const int year_for_leap = (month > 1) ? year + 1 : year;

	time_t rt = tm->tm_sec                             // Seconds
		+ 60 * (tm->tm_min                          // Minute = 60 seconds
				+ 60 * (tm->tm_hour                         // Hour = 60 minutes
					+ 24 * (month_day[month] + tm->tm_mday - 1  // Day = 24 hours
						+ 365 * (year - 70)                         // Year = 365 days
						+ (year_for_leap - 69) / 4                  // Every 4 years is     leap...
						- (year_for_leap - 1) / 100                 // Except centuries...
						+ (year_for_leap + 299) / 400)));           // Except 400s.
	return rt < 0 ? -1 : rt;
}

static std::string cp1251_to_utf8(const char *str)
{
	std::string res;
	int result_u, result_c;

	result_u = MultiByteToWideChar(1251, 0, str, -1, 0, 0);

	if (!result_u)
		return std::string();

	std::vector<wchar_t> ures(result_u);

	if(!MultiByteToWideChar(1251, 0, str, -1, ures.data(), result_u))
	{
		return std::string();
	}


	result_c = WideCharToMultiByte(CP_UTF8, 0, ures.data(), -1, 0, 0, 0, 0);

	if(!result_c)
	{
		return std::string();
	}

	std::vector<char> cres(result_c);

	if(!WideCharToMultiByte( CP_UTF8, 0, ures.data(), -1, cres.data(), result_c, 0, 0))
	{
		return std::string();
	}

	res.assign(cres.begin(), cres.end());
	return res;
}

void removeTrailingZeros(std::string& s)
{
	if(s.find('.') != std::string::npos)
	{
		size_t newLength = s.size() - 1;
		while(s[newLength] == '0')
		{
			newLength--;
		}

		if(s[newLength] == '.')
		{
			newLength--;
		}

		if(newLength != s.size() - 1)
		{
			s.resize(newLength + 1);
		}
	}
}

static std::pair<std::string, std::string> splitStringByTwo(const std::string& str, char c)
{
	auto charPosition = str.find(c);
	if(charPosition == std::string::npos)
		BOOST_THROW_EXCEPTION(ParameterError());

	auto first = str.substr(0, charPosition);
	auto second = str.substr(charPosition + 1, std::string::npos);

	return std::make_pair(first, second);
}

static std::string makeTransactionStringForOrder(const Order::Ptr& order, int transactionId)
{
	std::string account, clientCode;
	std::string accountString;
	try
	{
		std::tie(account, clientCode) = splitStringByTwo(order->account(), '#');
		accountString = "ACCOUNT=" + account + ";CLIENT_CODE=" + clientCode;
	}
	catch(ParameterError& e)
	{
		account = order->account();
		accountString = "ACCOUNT=" + account;
	}

	std::string classCode, secCode;
	try
	{
		std::tie(classCode, secCode) = splitStringByTwo(order->security(), '#');
	}
	catch(ParameterError& e)
	{
		e << errinfo_str("Invalid security ID, should be of the following form: CLASSCODE#SECCODE");
		throw e;
	}

	if((order->type() == Order::OrderType::Market) || (order->type() == Order::OrderType::Limit))
	{
		std::array<char, 1024> buf;
		std::array<char, 32> priceBuf;
		int priceLength = snprintf(priceBuf.data(), 32, "%f", order->price());
		std::string priceStr(priceBuf.data(), priceLength);
		removeTrailingZeros(priceStr);

		char orderTypeCode = (order->type() == Order::OrderType::Market ? 'M' : 'L');
		char operationCode = (order->operation() == Order::Operation::Buy ? 'B' : 'S');
		int stringSize = snprintf(buf.data(), 1024, "%s;TYPE=%c;TRANS_ID=%d;CLASSCODE=%s;SECCODE=%s;ACTION=NEW_ORDER;OPERATION=%c;PRICE=%s;QUANTITY=%d;",
				accountString.c_str(), orderTypeCode, transactionId, classCode.c_str(), secCode.c_str(), operationCode, priceStr.c_str(), order->quantity());
		return std::string(buf.data(), stringSize);
	}
}

static std::string makeKillOrderString(const Order::Ptr& order, int transactionId, double orderKey)
{
	std::string classCode, secCode;
	try
	{
		std::tie(classCode, secCode) = splitStringByTwo(order->security(), '#');
	}
	catch(ParameterError& e)
	{
		e << errinfo_str("Invalid security ID, should be of the following form: CLASSCODE#SECCODE");
		throw e;
	}

	std::array<char, 1024> buf;
	int stringSize = snprintf(buf.data(), 1024, "TRANS_ID=%d;CLASSCODE=%s;SECCODE=%s;ACTION=KILL_ORDER;ORDER_KEY=%lld",
			transactionId, classCode.c_str(), secCode.c_str(), (uint64_t)orderKey);
	return std::string(buf.data(), stringSize);
}

QuikBroker::QuikBroker(const std::string& dllPath, const std::string& quikPath,
				const std::list<std::string>& accounts) : m_quikPath(quikPath),
	m_run(false),
	m_connected(false),
	m_accounts(accounts)
{
	assert(!m_instance);
	m_instance = this;
	LOG_WITH(gs_logger, debug) << "Loading dll at path: " << dllPath;
	m_quik = std::unique_ptr<Trans2QuikApi>(new Trans2QuikApi(dllPath));
	LOG_WITH(gs_logger, info) << "QUIK DLL loaded";

	m_watchdogThread = boost::thread(std::bind(&QuikBroker::watchdogLoop, this));
}

QuikBroker::~QuikBroker()
{
	m_run = false;
	m_watchdogCv.notify_all();
	m_watchdogThread.join();
	m_instance = nullptr;
}

void QuikBroker::submitOrder(const Order::Ptr& order)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
	LOG_WITH(gs_logger, debug) << "New order: " << order->stringRepresentation();
	int transactionId = gs_id.fetch_add(1);
	m_unsubmittedOrders[transactionId] = order;

	auto transactionString = makeTransactionStringForOrder(order, transactionId);
	long result, extendedErrorCode;
	std::array<char, 1024> errorMessage;
	LOG_WITH(gs_logger, debug) << "Sending transaction: " << transactionString;
	result = m_quik->TRANS2QUIK_SEND_ASYNC_TRANSACTION((LPSTR)transactionString.c_str(), &extendedErrorCode,
			errorMessage.data(), errorMessage.size());
	if(result != Trans2QuikApi::TRANS2QUIK_SUCCESS)
		BOOST_THROW_EXCEPTION(ExternalApiError() << errinfo_str(std::string("Unable to send transaction: ") +
																errorMessage.data()));
}

void QuikBroker::cancelOrder(const Order::Ptr& order)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
	auto it = m_orderIdMap.find(order->localId());
	if(it == m_orderIdMap.end())
		BOOST_THROW_EXCEPTION(ParameterError() << errinfo_str("Unable to find Quik order ID"));
	LOG_WITH(gs_logger, debug) << "Cancel order: " << order->stringRepresentation();
	int transactionId = gs_id.fetch_add(1);

	auto transactionString = makeKillOrderString(order, transactionId, it->second);
	long result, extendedErrorCode;
	std::array<char, 1024> errorMessage;
	LOG_WITH(gs_logger, debug) << "Sending transaction: " << transactionString;
	result = m_quik->TRANS2QUIK_SEND_ASYNC_TRANSACTION((LPSTR)transactionString.c_str(), &extendedErrorCode,
			errorMessage.data(), errorMessage.size());
	if(result != Trans2QuikApi::TRANS2QUIK_SUCCESS)
		BOOST_THROW_EXCEPTION(ExternalApiError() << errinfo_str(std::string("Unable to send transaction: ") +
																errorMessage.data()));
}

void QuikBroker::registerReactor(const std::shared_ptr<Reactor>& reactor)
{
	m_reactors.push_back(reactor);
}

void QuikBroker::unregisterReactor(const std::shared_ptr<Reactor>& reactor)
{
	auto it = std::find(m_reactors.begin(), m_reactors.end(), reactor);
	if(it != m_reactors.end())
		m_reactors.erase(it);
}

Order::Ptr QuikBroker::order(int localId)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
	for(const auto& order : m_pendingOrders)
	{
		if((order.second)->localId() == localId)
			return order.second;
	}

	for(const auto& order : m_retiredOrders)
	{
		if(order->localId() == localId)
			return order;
	}

	for(const auto& order : m_unsubmittedOrders)
	{
		if((order.second)->localId() == localId)
			return order.second;
	}

	return Order::Ptr();
}

std::list<std::string> QuikBroker::accounts()
{
	return m_accounts;
}

bool QuikBroker::hasAccount(const std::string& account)
{
	auto it = std::find(m_accounts.begin(), m_accounts.end(), account);
	return it != m_accounts.end();
}

std::list<goldmine::Position> QuikBroker::positions()
{
	// TODO
	return std::list<goldmine::Position>();
}

void __stdcall QuikBroker::connectionStatusCallback(long event, long errorCode, LPSTR infoMessage)
{
	long extendedErrorCode, result;
	std::array<char, 1024> errorMessage;

	if(event == Trans2QuikApi::TRANS2QUIK_DLL_CONNECTED)
	{
		result = m_instance->m_quik->TRANS2QUIK_SET_TRANSACTIONS_REPLY_CALLBACK(&QuikBroker::transactionReplyCallback,
				&extendedErrorCode, errorMessage.data(), errorMessage.size());
		if(result != Trans2QuikApi::TRANS2QUIK_SUCCESS)
		{
			m_instance->m_connected = false;
			LOG_WITH(gs_logger, warning) << "Unable to set transaction reply callback: " << extendedErrorCode << "/" <<
					errorMessage.data();
			m_instance->m_quik->TRANS2QUIK_DISCONNECT(&extendedErrorCode, errorMessage.data(), errorMessage.size());
			return;
		}

		result = m_instance->m_quik->TRANS2QUIK_SUBSCRIBE_ORDERS((LPSTR)"", (LPSTR)"");
		if(result != Trans2QuikApi::TRANS2QUIK_SUCCESS)
		{
			m_instance->m_connected = false;
			LOG_WITH(gs_logger, warning) << "Unable to subscribe to orders stream";
			m_instance->m_quik->TRANS2QUIK_DISCONNECT(&extendedErrorCode, errorMessage.data(), errorMessage.size());
			return;
		}

		m_instance->m_quik->TRANS2QUIK_START_ORDERS(&QuikBroker::orderStatusCallback);

		result = m_instance->m_quik->TRANS2QUIK_SUBSCRIBE_TRADES((LPSTR)"", (LPSTR)"");
		if(result != Trans2QuikApi::TRANS2QUIK_SUCCESS)
		{
			m_instance->m_connected = false;
			LOG_WITH(gs_logger, warning) << "Unable to subscribe to trades stream";
			m_instance->m_quik->TRANS2QUIK_DISCONNECT(&extendedErrorCode, errorMessage.data(), errorMessage.size());
			return;
		}

		m_instance->m_quik->TRANS2QUIK_START_TRADES(&QuikBroker::tradeCallback);

		m_instance->m_connected = true;
	}
	else if((event == Trans2QuikApi::TRANS2QUIK_DLL_DISCONNECTED) ||
			(event == Trans2QuikApi::TRANS2QUIK_QUIK_DISCONNECTED))
	{
		LOG_WITH(gs_logger, warning) << "Disconnection event: " << event;
		m_instance->m_connected = false;
	}
}

void __stdcall QuikBroker::transactionReplyCallback(long result, long errorCode, long transactionReplyCode,
		DWORD transactionId, double orderNum, LPSTR transactionReplyMessage)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_instance->m_mutex);
	LOG_WITH(gs_logger, debug) << "Transaction reply: " << transactionId << "; result: " << result;

	auto orderIt = m_instance->m_unsubmittedOrders.find(transactionId);
	if(orderIt != m_instance->m_unsubmittedOrders.end())
	{
		auto order = orderIt->second;
		m_instance->m_unsubmittedOrders.erase(orderIt);

		if(result == Trans2QuikApi::TRANS2QUIK_SUCCESS)
		{
			m_instance->m_pendingOrders[orderNum] = order;
			m_instance->m_orderIdMap[order->localId()] = orderNum;
			order->updateState(Order::State::Submitted);
		}
		else
		{
			m_instance->m_retiredOrders.push_back(order);
			order->updateState(Order::State::Rejected);
			order->setMessage(cp1251_to_utf8(transactionReplyMessage));
		}

		m_instance->notifyOrderStateChange(order);
	}
}

void __stdcall QuikBroker::orderStatusCallback(long mode, DWORD transactionId, double number, LPSTR classCode,
		LPSTR secCode, double price, long balance, double volume, long isBuy, long status, long orderDescriptor)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_instance->m_mutex);
	LOG_WITH(gs_logger, debug) << "Order status: " << number << "; status: " << status << "; mode: " << mode;
	// TODO handle other modes
	if(mode == 0)
	{
		auto orderIt = m_instance->m_pendingOrders.find(number);
		if(orderIt == m_instance->m_pendingOrders.end())
		{
			LOG_WITH(gs_logger, warning) << "Incoming order status for unknown order";
			return;
		}

		auto order = orderIt->second;
		if((status != 1) && (status != 2)) // Order executed
		{
			if(balance == 0) // Order fully filled
			{
				order->updateState(Order::State::Executed);
				order->setExecutedQuantity(order->quantity());
				m_instance->m_pendingOrders.erase(orderIt);
				m_instance->m_retiredOrders.push_back(order);
			}
			else
			{
				order->updateState(Order::State::PartiallyExecuted);
				order->setExecutedQuantity(order->quantity() - balance);
			}
		}
		else if(status == 2)
		{
			order->updateState(Order::State::Cancelled);
			m_instance->m_pendingOrders.erase(orderIt);
			m_instance->m_retiredOrders.push_back(order);
		}
		else if(status == 1)
		{
			order->updateState(Order::State::Submitted);
			return; // TransactionReply callback already notified client that order is submitted... Probably I should move it here
		}
		m_instance->notifyOrderStateChange(order);
	}
}

void __stdcall QuikBroker::tradeCallback(long mode, double number, double orderNumber, LPSTR classCode, LPSTR secCode,
		double price, long quantity, double volume, long isSell, long tradeDescriptor)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_instance->m_mutex);
	if(mode == 0)
	{
		auto it = m_instance->m_pendingOrders.find(orderNumber);
		if(it == m_instance->m_pendingOrders.end())
		{
			LOG_WITH(gs_logger, warning) << "Incoming trade for unknown order: " << orderNumber;
			return;
		}
		auto order = it->second;
		Trade trade;
		trade.orderId = it->first;
		trade.quantity = quantity;
		trade.operation = (isSell ? Order::Operation::Sell : Order::Operation::Buy);
		trade.account = order->account();
		trade.security = order->security();

		long ymd = m_quik->TRANS2QUIK_TRADE_DATE_TIME(tradeDescriptor, 0);
		long hms = m_quik->TRANS2QUIK_TRADE_DATE_TIME(tradeDescriptor, 1);
		long us = m_quik->TRANS2QUIK_TRADE_DATE_TIME(tradeDescriptor, 2);
		struct tm tm;
		tm.tm_year = ymd / 10000 - 1900;
		tm.tm_mon = (ymd % 10000) / 100 - 1;
		tm.tm_mday = (ymd % 100);
		tm.tm_hour = hms / 10000;
		tm.tm_min = (hms % 10000) / 100;
		tm.tm_sec = (hms % 100);
		trade.timestamp = mkgmtime(&tm);
		trade.useconds = us;

		m_instance->notifyTrade(trade);
	}
}

void QuikBroker::watchdogLoop()
{
	long extendedErrorCode, result;
	std::array<char, 1024> errorMessage;
	result = m_quik->TRANS2QUIK_SET_CONNECTION_STATUS_CALLBACK(&QuikBroker::connectionStatusCallback, &extendedErrorCode,
			errorMessage.data(), errorMessage.size());
	if(result != Trans2QuikApi::TRANS2QUIK_SUCCESS)
	{
		LOG_WITH(gs_logger, warning) << "Unable to set connection callback: " << extendedErrorCode << "/" <<
				errorMessage.data();
		return;
	}
	m_run = true;
	while(m_run)
	{
		boost::unique_lock<boost::mutex> lock(m_watchdogMutex);
		m_watchdogCv.timed_wait(lock, boost::posix_time::time_duration(0, 0, 10));
		if(!m_connected)
		{
			result = m_quik->TRANS2QUIK_CONNECT(m_quikPath.c_str(), &extendedErrorCode, errorMessage.data(),
					errorMessage.size());
			if(result != Trans2QuikApi::TRANS2QUIK_SUCCESS)
			{
				LOG_WITH(gs_logger, warning) << "Unable to connect: " << extendedErrorCode << "/" <<
						errorMessage.data();
			}
		}
	}
}

void QuikBroker::notifyOrderStateChange(const goldmine::Order::Ptr& order)
{
	for(const auto& reactor : m_reactors)
	{
		reactor->orderCallback(order);
	}
}

void QuikBroker::notifyTrade(const goldmine::Trade& trade)
{
	for(const auto& reactor : m_reactors)
	{
		reactor->tradeCallback(trade);
	}
}
