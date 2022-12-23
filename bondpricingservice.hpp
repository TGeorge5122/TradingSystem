/**
 * bondpricingservice.hpp
 */
#ifndef BOND_PRICING_SERVICE_HPP
#define BOND_PRICING_SERVICE_HPP

#include "soa.hpp"
#include "products.hpp"
#include "pricingservice.hpp"
#include "bonduniverseservice.hpp"
#include "util.hpp"

class BondPricingService : public PricingService<Bond>
{
private:

	vector<string> product_vec;
	vector<Bond> products;
	vector<double> mids;
	vector<double> spreads;
	//vector<Price<Bond> > price_vec;
	vector<ServiceListener<Price<Bond> >* > listeners;

public:

	// Get data on our service given a key
	Price<Bond> GetData(string product_id);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Price<Bond>& ob);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<Price<Bond> >* listener);

	// Get all listeners on the Service.
	const vector<ServiceListener<Price<Bond> >* >& GetListeners() const;

};

// Connector subscribing data from marketdata.txt to BondMarketDataService.
class BondPricingConnector : public Connector<Price<Bond> >
{

private:

	BondPricingService* prc_service;
	BondUniverseService* uni_service;

public:

	BondPricingConnector(BondPricingService*, BondUniverseService*);

	// Publish data to the Connector
	void Publish(Price<Bond>&);

	// Subscribe to prices.txt
	void Subscribe();

};

// Get data on our service given a key
Price<Bond> BondPricingService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return Price<Bond>(products[index], mids[index], spreads[index]);
	//return price_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondPricingService::OnMessage(Price<Bond>& p) {

	int index = find(product_vec.begin(), product_vec.end(), p.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		products.erase(products.begin() + index);
		mids.erase(mids.begin() + index);
		spreads.erase(spreads.begin() + index);
	}

	product_vec.push_back(p.GetProduct().GetProductId());
	products.push_back(p.GetProduct());
	mids.push_back(p.GetMid());
	spreads.push_back(p.GetBidOfferSpread());
	//price_vec.push_back(p);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(p);
	}
}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondPricingService::AddListener(ServiceListener<Price<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<Price<Bond> >*>& BondPricingService::GetListeners() const {
	return listeners;
}

BondPricingConnector::BondPricingConnector(BondPricingService* prc_service_, BondUniverseService* uni_service_) {
	prc_service = prc_service_;
	uni_service = uni_service_;
}

void BondPricingConnector::Publish(Price<Bond>& data) {}

void BondPricingConnector::Subscribe() {
	ifstream input_file;
	input_file.open("prices.txt");
	string update;

	Bond b;
	double bid;
	double offer;

	while (getline(input_file, update)) {

		vector<string> update_split = split(update, ",");

		b = uni_service->GetData(update_split[0]);
		bid = TreasuryPrices(update_split[1]).toDouble();
		offer = TreasuryPrices(update_split[2]).toDouble();

		Price<Bond> p(b, (bid + offer) / 2, offer - bid);
		prc_service->OnMessage(p);
	}
}

#endif
