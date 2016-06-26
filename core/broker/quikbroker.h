#ifndef QUIKBROKER_H
#define QUIKBROKER_H

#include "broker/broker.h"
#include "trans2quik/trans2quik.h"

#include <boost/thread.hpp>

#include <atomic>

class QuikBroker : public goldmine::Broker
{
public:
	QuikBroker(const std::string& dllPath, const std::string& quikPath,
				const std::list<std::string>& accounts);
	virtual ~QuikBroker();

	virtual void submitOrder(const goldmine::Order::Ptr& order);
	virtual void cancelOrder(const goldmine::Order::Ptr& order);

	virtual void registerReactor(const std::shared_ptr<Reactor>& reactor);
	virtual void unregisterReactor(const std::shared_ptr<Reactor>& reactor);

	virtual goldmine::Order::Ptr order(int localId);

	virtual std::list<std::string> accounts();
	virtual bool hasAccount(const std::string& account);

	virtual std::list<goldmine::Position> positions();

private:
	static void __stdcall connectionStatusCallback(long event, long errorCode, LPSTR infoMessage);
	static void __stdcall transactionReplyCallback(long result, long errorCode, long transactionReplyCode,
			DWORD transactionId, double orderNum, LPSTR transactionReplyMessage);
	static void __stdcall orderStatusCallback(long mode, DWORD transactionId, double number,
			LPSTR classCode, LPSTR secCode, double price, long balance, double volume, long isBuy, long status,
			long orderDescriptor);
	static void __stdcall tradeCallback(long mode, double number, double orderNumber, LPSTR classCode, LPSTR secCode,
			double price, long quantity, double volume, long isSell, long tradeDescriptor);

	void watchdogLoop();
	void notifyOrderStateChange(const goldmine::Order::Ptr& order);
	void notifyTrade(const goldmine::Trade& trade);

private:
	static QuikBroker* m_instance;
	static std::unique_ptr<Trans2QuikApi> m_quik;

	std::string m_quikPath;

	boost::thread m_watchdogThread;
	boost::mutex m_watchdogMutex;
	boost::condition_variable m_watchdogCv;

	std::atomic_bool m_run;
	std::atomic_bool m_connected;

	std::list<std::string> m_accounts;
	boost::recursive_mutex m_mutex;

	std::map<int, goldmine::Order::Ptr> m_unsubmittedOrders;
	std::map<double, goldmine::Order::Ptr> m_pendingOrders;
	std::list<goldmine::Order::Ptr> m_retiredOrders;

	std::map<int, double> m_orderIdMap;
	std::vector<std::shared_ptr<Reactor>> m_reactors;
};

#endif // QUIKBROKER_H
