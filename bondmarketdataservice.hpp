/**
 * bondmarketdataservice.hpp
 */
#ifndef BONDMARKET_DATA_SERVICE_HPP
#define BONDMARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "soa.hpp"
#include "treasuryprices.hpp"
#include "marketdataservice.hpp"
#include "bonduniverseservice.hpp"
#include "products.hpp"
#include "util.hpp"

class BondMarketDataService : public Service<string,OrderBook <Bond> >
{
private:

	vector<string> product_vec;
	vector<OrderBook<Bond> > ob_vec;
	//map<string, OrderBook<Bond> > product_ob_map;
	vector<ServiceListener<OrderBook<Bond> >* > listeners;

public:

	// Get data on our service given a key
	OrderBook<Bond> GetData(string product_id);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(OrderBook<Bond>& ob);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<OrderBook<Bond> >* listener);

	// Get all listeners on the Service.
	const vector<ServiceListener<OrderBook<Bond> >* >& GetListeners() const;

	// Get the best bid/offer order
	const BidOffer GetBestBidOffer(const string &productId);

	// Aggregate the order book
	OrderBook<Bond> AggregateDepth(const string &productId);

};

// Connector subscribing data from marketdata.txt to BondMarketDataService.
class BondMarketDataConnector : public Connector<OrderBook<Bond> >
{

private:

	BondMarketDataService* md_service;
	BondUniverseService* uni_service;

public:

	BondMarketDataConnector(BondMarketDataService*, BondUniverseService*);

	// Publish data to the Connector
	void Publish(OrderBook<Bond>&) ;

	// Subscribe to marketdata.txt
	void Subscribe();

};

// Get data on our service given a key
OrderBook<Bond> BondMarketDataService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return ob_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondMarketDataService::OnMessage(OrderBook<Bond>& ob) {

	int index = find(product_vec.begin(), product_vec.end(), ob.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		ob_vec.erase(ob_vec.begin() + index);
	}

	product_vec.push_back(ob.GetProduct().GetProductId());
	ob_vec.push_back(ob);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(ob);
	}
}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondMarketDataService::AddListener(ServiceListener<OrderBook<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<OrderBook<Bond> >*>& BondMarketDataService::GetListeners() const {
	return listeners;
}

// Get the best bid/offer order
const BidOffer BondMarketDataService::GetBestBidOffer(const string& productId) {

	int index = find(product_vec.begin(), product_vec.end(), productId) - product_vec.begin();
	vector<Order> bid_stack = ob_vec[index].GetBidStack();
	vector<Order> offer_stack = ob_vec[index].GetOfferStack();

	Order best_bid = bid_stack.front();
	Order best_offer = offer_stack.front();

	for (int i = 1; i < bid_stack.size(); i++) {

		if (best_bid.GetPrice() < bid_stack[i].GetPrice()) {
			best_bid = bid_stack[i];
		}

	}

	for (int i = 1; i < offer_stack.size(); i++) {

		if (best_offer.GetPrice() > offer_stack[i].GetPrice()) {
			best_offer = offer_stack[i];
		}

	}

	return BidOffer(best_bid, best_offer);
}

// Aggregate the order book
OrderBook<Bond> BondMarketDataService::AggregateDepth(const string& productId) {

	int index = find(product_vec.begin(), product_vec.end(), productId) - product_vec.begin();
	vector<Order> bid_stack = ob_vec[index].GetBidStack();
	vector<Order> offer_stack = ob_vec[index].GetOfferStack();

	map<double, long> price_qty;

	vector<Order> agg_bid_stack;
	vector<Order> agg_offer_stack;


	for (std::vector<Order>::iterator it = bid_stack.begin(); it != bid_stack.end(); it++) {

		if (price_qty.find(it->GetPrice()) != price_qty.end()) {
			price_qty[it->GetPrice()] += it->GetQuantity();
		}
		else {
			price_qty[it->GetPrice()] = it->GetQuantity();
		}

	}

	for (std::map<double,long>::iterator it = price_qty.begin(); it != price_qty.end(); it++) {
		agg_bid_stack.push_back(Order(it->first, it->second, BID));
	}

	price_qty.clear();

	for (std::vector<Order>::iterator it = offer_stack.begin(); it != offer_stack.end(); it++) {

		if (price_qty.find(it->GetPrice()) != price_qty.end()) {
			price_qty[it->GetPrice()] += it->GetQuantity();
		}
		else {
			price_qty[it->GetPrice()] = it->GetQuantity();
		}

	}

	for (std::map<double, long>::iterator it = price_qty.begin(); it != price_qty.end(); it++) {
		agg_offer_stack.push_back(Order(it->first, it->second, OFFER));
	}

	return OrderBook<Bond>(ob_vec[index].GetProduct(), agg_bid_stack, agg_offer_stack);

}


BondMarketDataConnector::BondMarketDataConnector(BondMarketDataService* md_service_, BondUniverseService* uni_service_) {
	md_service = md_service_;
	uni_service = uni_service_;
}

void BondMarketDataConnector::Publish(OrderBook<Bond>& data) {}

void BondMarketDataConnector::Subscribe() {

	ifstream input_file;
	input_file.open("marketdata.txt");
	string update;

	vector<Order> bid_stack, offer_stack;

	string product;
	double price;
	long quantity;

	while (getline(input_file, update)) {

		bid_stack.clear();
		offer_stack.clear();

		vector<string> update_split = split(update, ",");

		for (int i = 0; i < update_split.size() - 2; i += 3) {

			//First 3 are product id, mid price, MID
			if (i == 0) {	

				product = update_split[i];

				continue;
			}

			//Rest are price, quantity, Side
			TreasuryPrices tp(update_split[i]);
			price = tp.toDouble();
			std::sscanf(update_split[i + 1].c_str(), "%d", &quantity);

			if (update_split[i + 2] == "BID") {
				bid_stack.push_back(Order(price, quantity, BID));
			}
			else {
				offer_stack.push_back(Order(price, quantity, OFFER));
			}
		}

		Bond b = uni_service->GetData(product);
		OrderBook<Bond> ob(b, bid_stack, offer_stack);

		md_service->OnMessage(ob);
	}
}

#endif
