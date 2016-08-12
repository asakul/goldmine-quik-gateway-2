#ifndef PAPERBROKER_H
#define PAPERBROKER_H

#include "broker/broker.h"

#include "core/quotetable.h"

#include <boost/thread/recursive_mutex.hpp>

class PaperBroker : public goldmine::Broker
{
public:
	PaperBroker(double startCash, const QuoteTable::Ptr& table);
	virtual ~PaperBroker();

	virtual void submitOrder(const goldmine::Order::Ptr& order);
	virtual void cancelOrder(const goldmine::Order::Ptr& order);

	virtual void registerReactor(const std::shared_ptr<Reactor>& reactor);
	virtual void unregisterReactor(const std::shared_ptr<Reactor>& reactor);

	virtual goldmine::Order::Ptr order(int localId);

	virtual std::list<std::string> accounts();
	virtual bool hasAccount(const std::string& account);

	virtual std::list<goldmine::Position> positions();

private:
	void addPendingOrder(const goldmine::Order::Ptr& order);
	void unsubscribeFromTickerIfNeeded(const std::string& ticker);
	void incomingTick(const std::string& ticker, const goldmine::Tick& tick);

private:
	void orderStateUpdated(const goldmine::Order::Ptr& order);
	void emitTrade(const goldmine::Trade& trade);
	void executeBuyAt(const goldmine::Order::Ptr& order, const goldmine::decimal_fixed& price, uint64_t timestamp, uint32_t useconds);
	void executeSellAt(const goldmine::Order::Ptr& order, const goldmine::decimal_fixed& price, uint64_t timestamp, uint32_t useconds);

private:
	std::vector<std::shared_ptr<Reactor>> m_reactors;
	std::list<goldmine::Order::Ptr> m_pendingOrders;
	std::list<goldmine::Order::Ptr> m_allOrders;
	std::map<std::string, int> m_portfolio;
	double m_cash;
	QuoteTable::Ptr m_table;
	boost::recursive_mutex m_mutex;
};

#endif // PAPERBROKER_H
