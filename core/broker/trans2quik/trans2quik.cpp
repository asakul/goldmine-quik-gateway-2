/*
 * trans2quik.cpp
 */

#include "trans2quik.h"
#include <exceptions.h>
#include <string>

#define LOAD_SYMBOL(sym) sym = (decltype(sym))GetProcAddress((HMODULE)m_handle, #sym); if(!sym) throw std::runtime_error("Unable to obtain symbol: " #sym +\
		std::string(":") + std::to_string(GetLastError()));
#define LOAD_SYMBOL_2(sym, args) sym = (decltype(sym))GetProcAddress((HMODULE)m_handle, "_" #sym "@" #args); if(!sym) throw std::runtime_error("Unable to obtain symbol: " #sym +\
		std::string(":") + std::to_string(GetLastError()));

Trans2QuikApi::Trans2QuikApi(const std::string& dll)
{
	m_handle = LoadLibraryEx(dll.c_str(), NULL, 0);
	if(!m_handle)
		throw std::runtime_error("Unable to load dll");

	LOAD_SYMBOL_2(TRANS2QUIK_CONNECT, 16);
	LOAD_SYMBOL_2(TRANS2QUIK_DISCONNECT, 12);
	LOAD_SYMBOL_2(TRANS2QUIK_IS_QUIK_CONNECTED, 12);
	LOAD_SYMBOL_2(TRANS2QUIK_IS_DLL_CONNECTED, 12);
	LOAD_SYMBOL_2(TRANS2QUIK_SEND_SYNC_TRANSACTION, 36);
	LOAD_SYMBOL_2(TRANS2QUIK_SEND_ASYNC_TRANSACTION, 16);
	LOAD_SYMBOL_2(TRANS2QUIK_SET_CONNECTION_STATUS_CALLBACK, 16);
	LOAD_SYMBOL_2(TRANS2QUIK_SET_TRANSACTIONS_REPLY_CALLBACK, 16);
	LOAD_SYMBOL_2(TRANS2QUIK_SUBSCRIBE_ORDERS, 8);
	LOAD_SYMBOL_2(TRANS2QUIK_SUBSCRIBE_TRADES, 8);
	LOAD_SYMBOL_2(TRANS2QUIK_START_ORDERS, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_START_TRADES, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_UNSUBSCRIBE_ORDERS, 0);
	LOAD_SYMBOL_2(TRANS2QUIK_UNSUBSCRIBE_TRADES, 0);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_DATE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_SETTLE_DATE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_TIME, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_IS_MARGINAL, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_ACCRUED_INT, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_YIELD, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_TS_COMMISSION, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_CLEARING_CENTER_COMMISSION, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_EXCHANGE_COMMISSION, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_TRADING_SYSTEM_COMMISSION, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_PRICE2, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_REPO_RATE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_REPO_VALUE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_REPO2_VALUE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_ACCRUED_INT2, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_REPO_TERM, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_START_DISCOUNT, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_LOWER_DISCOUNT, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_UPPER_DISCOUNT, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_BLOCK_SECURITIES, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_DATE_TIME, 8);

	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_CURRENCY, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_SETTLE_CURRENCY, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_SETTLE_CODE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_ACCOUNT, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_BROKERREF, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_CLIENT_CODE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_USERID, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_FIRMID, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_PARTNER_FIRMID, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_EXCHANGE_CODE, 4);
	LOAD_SYMBOL_2(TRANS2QUIK_TRADE_STATION_ID, 4);
}

Trans2QuikApi::~Trans2QuikApi()
{
	CloseHandle(m_handle);
}
