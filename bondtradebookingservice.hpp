/**
 * bondtradebookingservice.hpp
 */
#ifndef BOND_TRADE_BOOKING_SERVICE_HPP
#define BOND_TRADE_BOOKING_SERVICE_HPP

#include "treasuryprices.hpp"
#include "tradebookingservice.hpp"
#include "bonduniverseservice.hpp"
#include "util.hpp"

class BondTradeBookingService : public TradeBookingService<Bond>
{
private:

	vector<string> product_vec;
	vector<Trade<Bond> > trade_vec;
	vector<ServiceListener<Trade<Bond> >* > listeners;

public:

	// Get data on our service given a key
	Trade<Bond> GetData(string product_id);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Trade<Bond>& ob);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<Trade<Bond> >* listener);

	// Get all listeners on the Service.
	const vector<ServiceListener<Trade<Bond> >* >& GetListeners() const;

	// Book the trade
	void BookTrade(Trade<Bond>& trade);

};

// Connector subscribing data from marketdata.txt to BondMarketDataService.
class BondTradeBookingConnector : public Connector<Trade<Bond> >
{

private:

	BondTradeBookingService* book_trade_service;
	BondUniverseService* uni_service;

public:

	BondTradeBookingConnector(BondTradeBookingService*, BondUniverseService*);

	// Publish data to the Connector
	void Publish(Trade<Bond>&);

	// Subscribe to prices.txt
	void Subscribe();

};

// Get data on our service given a key
Trade<Bond> BondTradeBookingService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return trade_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondTradeBookingService::OnMessage(Trade<Bond>& p) {
	BookTrade(p);
}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondTradeBookingService::AddListener(ServiceListener<Trade<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<Trade<Bond> >*>& BondTradeBookingService::GetListeners() const {
	return listeners;
}

// Book the trade
void BondTradeBookingService::BookTrade(Trade<Bond>& trade) {

	int index = find(product_vec.begin(), product_vec.end(), trade.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		trade_vec.erase(trade_vec.begin() + index);
	}

	product_vec.push_back(trade.GetProduct().GetProductId());
	trade_vec.push_back(trade);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(trade);
	}
}

BondTradeBookingConnector::BondTradeBookingConnector(BondTradeBookingService* book_trade_service_, BondUniverseService* uni_service_) {
	book_trade_service = book_trade_service_;
	uni_service = uni_service_;
}

void BondTradeBookingConnector::Publish(Trade<Bond>& data) {}

void BondTradeBookingConnector::Subscribe() {
	ifstream input_file;
	input_file.open("trades.txt");
	string update;

	Bond b;
	TreasuryPrices price;
	long quantity;

	while (getline(input_file, update)) {

		vector<string> update_split = split(update, ",");

		b = uni_service->GetData(update_split[0]);
		price = TreasuryPrices(update_split[2]);
		std::sscanf(update_split[4].c_str(), "%d", &quantity);

		Trade<Bond> t(b, update_split[1], price.toDouble(), update_split[3], quantity, update_split[5] == "BUY" ? BUY : SELL);
		book_trade_service->OnMessage(t);

	}
}

#endif
