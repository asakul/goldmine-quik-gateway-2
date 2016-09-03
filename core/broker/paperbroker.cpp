#include "paperbroker.h"

#include "log.h"

using namespace goldmine;
namespace sp = std::placeholders;

static logger_t gs_logger(boost::log::keywords::channel = "paper_broker");

PaperBroker::PaperBroker(double startCash, const QuoteTable::Ptr& table) : m_cash(startCash),
	m_table(table)
{
	m_table->setTickCallback(std::bind(&PaperBroker::incomingTick, this, sp::_1, sp::_2));
}

PaperBroker::~PaperBroker()
{
}

void PaperBroker::submitOrder(const goldmine::Order::Ptr& order)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
	m_allOrders.push_back(order);
	LOG_WITH(gs_logger, info) << "VirtualBroker: submitted order: " << order->stringRepresentation();

	if(order->type() == Order::OrderType::Market)
	{
		auto bidTick = m_table->lastQuote(order->security(), goldmine::Datatype::BestBid);
		auto offerTick = m_table->lastQuote(order->security(), goldmine::Datatype::BestOffer);
		auto bid = bidTick.value.toDouble();
		auto offer = offerTick.value.toDouble();

		LOG_WITH(gs_logger, debug) << bid << "/" << offer;

		if(order->operation() == Order::Operation::Buy)
		{
			if(offer == 0)
			{
				LOG_WITH(gs_logger, debug) << "No offers";
				order->updateState(Order::State::Rejected);
			}
			else
			{
				double volume = offer * order->quantity();
				if(m_cash < volume)
				{
					LOG_WITH(gs_logger, warning) << "Not enough cash";
					order->updateState(Order::State::Rejected);
				}
				else
				{
					LOG_WITH(gs_logger, debug) << "Order OK";
					executeBuyAt(order, offerTick.value, offerTick.timestamp, offerTick.useconds);
				}
			}
			orderStateUpdated(order);
		}
		else
		{
			if(bid == 0)
			{
				LOG_WITH(gs_logger, debug) << "No bids";
				order->updateState(Order::State::Rejected);
			}
			else
			{
				executeSellAt(order, bidTick.value, bidTick.timestamp, bidTick.useconds);
			}
			orderStateUpdated(order);
		}
	}
	else if(order->type() == Order::OrderType::Limit)
	{
		LOG_WITH(gs_logger, debug) << "Limit order";
		auto bidTick = m_table->lastQuote(order->security(), goldmine::Datatype::BestBid);
		auto offerTick = m_table->lastQuote(order->security(), goldmine::Datatype::BestOffer);
		auto bid = bidTick.value.toDouble();
		auto offer = offerTick.value.toDouble();

		LOG_WITH(gs_logger, debug) << bid << "/" << offer;

		if(order->operation() == Order::Operation::Buy)
		{
			if((offerTick.value <= order->price()) && (offer != 0))
			{
				double volume = offer * order->quantity();
				if(m_cash < volume)
				{
					LOG_WITH(gs_logger, debug) << "Not enough cash";
					order->updateState(Order::State::Rejected);
				}
				else
				{
					LOG_WITH(gs_logger, debug) << "Order OK";
					executeBuyAt(order, offerTick.value, offerTick.timestamp, offerTick.useconds);
				}
			}
			else
			{
				addPendingOrder(order);
			}
		}
		else if(order->operation() == Order::Operation::Sell)
		{
			if((bid != 0) && (bidTick.value >= order->price()))
			{
				LOG_WITH(gs_logger, debug) << "Order OK";
				executeSellAt(order, bidTick.value, bidTick.timestamp, bidTick.useconds);
			}
			else
			{
				addPendingOrder(order);
			}
		}
		orderStateUpdated(order);
	}
	else
	{
		LOG_WITH(gs_logger, warning) << "Requested unsupported order type";
	}
	LOG_WITH(gs_logger, info) << "Cash: " << m_cash;
}

void PaperBroker::cancelOrder(const goldmine::Order::Ptr& order)
{

}

void PaperBroker::registerReactor(const std::shared_ptr<Reactor>& reactor)
{
	m_reactors.push_back(reactor);
}

void PaperBroker::unregisterReactor(const std::shared_ptr<Reactor>& reactor)
{

}

Order::Ptr PaperBroker::order(int localId)
{
	auto it = std::find_if(m_pendingOrders.begin(), m_pendingOrders.end(), [&](const Order::Ptr& order) { return order->localId() == localId; } );
	if(it == m_pendingOrders.end())
	{
		it = std::find_if(m_allOrders.begin(), m_allOrders.end(), [&](const Order::Ptr& order) { return order->localId() == localId; } );
		if(it == m_allOrders.end())
			return Order::Ptr();
	}

	return *it;
}

std::list<std::string> PaperBroker::accounts()
{
	std::list<std::string> result;
	result.push_back("demo");
	return result;
}

bool PaperBroker::hasAccount(const std::string& account)
{
	return account == "demo";
}

void PaperBroker::addPendingOrder(const Order::Ptr& order)
{
	order->updateState(Order::State::Submitted);
	m_pendingOrders.push_back(order);
	m_table->enableTicker(order->security());
}

void PaperBroker::unsubscribeFromTickerIfNeeded(const std::string& ticker)
{
	auto it = std::find_if(m_pendingOrders.begin(), m_pendingOrders.end(), [&](const Order::Ptr& order) { return order->security() == ticker; } );
	if(it == m_pendingOrders.end())
	{
		m_table->disableTicker(ticker);
	}
}

void PaperBroker::incomingTick(const std::string& ticker, const goldmine::Tick& tick)
{
	boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
	LOG_WITH(gs_logger, debug) << "VirtualBroker::incomingTick: " << ticker;
	auto it = m_pendingOrders.begin();
	while(it != m_pendingOrders.end())
	{
		auto next = it;
		++next;

		auto order = *it;
		LOG_WITH(gs_logger, debug) << "Order: " << order->stringRepresentation();
		if(next != m_pendingOrders.end())
		{
			LOG_WITH(gs_logger, debug) << "Next Order: " << (*next)->stringRepresentation();
		}
		if(order->security() == ticker)
		{
			if((order->operation() == Order::Operation::Buy) && (tick.value <= order->price()) &&
					((tick.datatype == (int)goldmine::Datatype::BestOffer) || (tick.datatype == (int)goldmine::Datatype::Price)))
			{
				executeBuyAt(order, order->price(), tick.timestamp, tick.useconds);
				m_pendingOrders.erase(it);
				unsubscribeFromTickerIfNeeded(order->security());
				orderStateUpdated(order);
			}
			else if((order->operation() == Order::Operation::Sell) && (tick.value >= order->price()) &&
					((tick.datatype == (int)goldmine::Datatype::BestBid) || (tick.datatype == (int)goldmine::Datatype::Price)))
			{
				executeSellAt(order, order->price(), tick.timestamp, tick.useconds);
				m_pendingOrders.erase(it);
				unsubscribeFromTickerIfNeeded(order->security());
				orderStateUpdated(order);
			}
		}

		it = next;
	}
}

void PaperBroker::orderStateUpdated(const Order::Ptr& order)
{
	LOG_WITH(gs_logger, debug) << "VirtualBroker::orderStateUpdated: " << order->stringRepresentation();
	for(const auto& c : m_reactors)
	{
		c->orderCallback(order);
	}
}

void PaperBroker::emitTrade(const Trade& trade)
{
	for(const auto& c : m_reactors)
	{
		c->tradeCallback(trade);
	}
}

void PaperBroker::executeBuyAt(const Order::Ptr& order, const goldmine::decimal_fixed& price, uint64_t timestamp, uint32_t useconds)
{
	LOG_WITH(gs_logger, info) << "VB: executeBuyAt: " << price.toDouble();
	double volume = price.toDouble() * order->quantity();
	order->updateState(Order::State::Executed);
	m_portfolio[order->security()] += order->quantity();
	m_cash -= volume;

	Trade trade;
	trade.orderId = order->clientAssignedId();
	trade.account = order->account();
	trade.price = price.toDouble();
	trade.volume = volume;
	trade.volumeCurrency = "TEST";
	trade.quantity = order->quantity();
	trade.operation = order->operation();
	trade.security = order->security();
	trade.timestamp = timestamp;
	trade.useconds = useconds;
	trade.signalId = order->signalId();
	emitTrade(trade);
}

void PaperBroker::executeSellAt(const Order::Ptr& order, const goldmine::decimal_fixed& price, uint64_t timestamp, uint32_t useconds)
{
	LOG_WITH(gs_logger, info) << "VB: executeSellAt: " << price.toDouble();
	double volume = price.toDouble() * order->quantity();
	m_cash += volume;
	m_portfolio[order->security()] -= order->quantity();
	order->updateState(Order::State::Executed);

	Trade trade;
	trade.orderId = order->clientAssignedId();
	trade.account = order->account();
	trade.price = price.toDouble();
	trade.volume = volume;
	trade.volumeCurrency = "TEST";
	trade.quantity = order->quantity();
	trade.operation = order->operation();
	trade.security = order->security();
	trade.timestamp = timestamp;
	trade.useconds = useconds;
	trade.signalId = order->signalId();
	emitTrade(trade);
}

std::list<Position> PaperBroker::positions()
{
	std::list<Position> result;
	for(auto it = m_portfolio.begin(); it != m_portfolio.end(); ++it)
	{
		result.push_back(Position {it->first, it->second} );
	}
	return result;
}
