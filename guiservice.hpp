/**
 * guiservice.hpp
 */
#ifndef GUI_SERVICE_HPP
#define GUI_SERVICE_HPP

#include <fstream>
#include "soa.hpp"
#include "bondpricingservice.hpp"

class PriceTime
{
private:
	//Price<Bond> p;
	Bond b;
	double mid;
	double spread;
	long ms;
public:
	PriceTime(Bond, double, double, long);
	Price<Bond> GetPrice();
	long GetTime();
};

PriceTime::PriceTime(Bond b_, double mid_, double spread_, long ms_) {
	b = b_;
	mid = mid_;
	spread = spread_;
	ms = ms_;
}

Price<Bond> PriceTime::GetPrice() {
	return Price<Bond>(b,mid,spread);
}

long PriceTime::GetTime() {
	return ms;
}

 //Forward declaration for use in GUIService
class GUIConnector;

class GUIService : public Service<string, Price<Bond> >{
private:
	
	vector<string> product_vec;
	vector<Bond> products;
	vector<double> mids;
	vector<double> spreads;
	//vector<Price<Bond> > price_vec;
	vector<ServiceListener<Price<Bond> >* > listeners;
	GUIConnector* gui_connector;
	int max_updates;

public:

	GUIService(GUIConnector*);

	// Get data on our service given a key
	Price<Bond> GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Price<Bond>&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<Price<Bond> >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<Price<Bond> >* >& GetListeners() const;

	// Publish two-way prices
	void PublishPrice(Price<Bond>& prc, long ms);

};

class BondPricingServiceToGUIListener : public ServiceListener<Price<Bond> >
{
private:
	GUIService* gui_service;
	long throttle;
	long prev_msg_ms;
	long current_msg_ms;

public:

	BondPricingServiceToGUIListener(GUIService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Price<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Price<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Price<Bond>& data);

};

// Connector subscribing data from marketdata.txt to BondMarketDataService.
class GUIConnector : public Connector<PriceTime >
{

private:

	GUIService* gui_service;

public:

	GUIConnector();

	GUIConnector(GUIService*);

	// Publish data to the Connector
	void Publish(PriceTime&);

	void Subscribe();

	setGUIService(GUIService*);

};

GUIService::GUIService(GUIConnector* gui_connector_) {
	gui_connector = gui_connector_;
	max_updates = 100;
}

// Get data on our service given a key
Price<Bond> GUIService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return Price<Bond>(products[index], mids[index], spreads[index]);
	//return price_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void GUIService::OnMessage(Price <Bond>& prc) {
	
	int index = find(product_vec.begin(), product_vec.end(), prc.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		products.erase(products.begin() + index);
		mids.erase(mids.begin() + index);
		spreads.erase(spreads.begin() + index);
	}

	product_vec.push_back(prc.GetProduct().GetProductId());
	products.push_back(prc.GetProduct());
	mids.push_back(prc.GetMid());
	spreads.push_back(prc.GetBidOfferSpread());
	//price_vec.push_back(p);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(prc);
	}

}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void GUIService::AddListener(ServiceListener<Price<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<Price<Bond> >*>& GUIService::GetListeners() const {
	return listeners;
}

// Publish two-way prices
void GUIService::PublishPrice(Price<Bond>& prc, long ms) {

	OnMessage(prc);

	if (max_updates <= 0) {
		return;
	}
	max_updates--;

	PriceTime pt(prc.GetProduct(), prc.GetMid(), prc.GetBidOfferSpread(), ms);
	gui_connector->Publish(pt);
}

BondPricingServiceToGUIListener::BondPricingServiceToGUIListener(GUIService* gui_service_) {
	gui_service = gui_service_;
	throttle = 300;
	prev_msg_ms = 0;
	current_msg_ms = 0;
}

// Listener callback to process an add event to the Service
void BondPricingServiceToGUIListener::ProcessAdd(Price<Bond>& data) {

	current_msg_ms += 100;

	//Making up milliseconds to get around boost::chrono
	//Treating each loop as 100 ms and each msg update as 100 ms
	while (current_msg_ms - prev_msg_ms < throttle) {
		current_msg_ms += 100;
	}

	prev_msg_ms = current_msg_ms;
	gui_service->PublishPrice(data, current_msg_ms);
}

// Listener callback to process a remove event to the Service
void BondPricingServiceToGUIListener::ProcessRemove(Price<Bond>& data) {
	current_msg_ms += 100;
}

// Listener callback to process an update event to the Service
void BondPricingServiceToGUIListener::ProcessUpdate(Price<Bond>& data) {
	current_msg_ms += 100;
}

GUIConnector::GUIConnector() {
	std::remove("gui.txt");

}

GUIConnector::GUIConnector(GUIService* gui_service_) {
	gui_service = gui_service_;
	std::remove("gui.txt");
}

// Publish data to the Connector
void GUIConnector::Publish(PriceTime& ps) {

	Price<Bond> p = ps.GetPrice();

	std::ofstream output;
	output.open("gui.txt", ios::app);
	output << ps.GetTime() << "," << p.GetProduct().GetProductId() << "," << p.GetMid() << "," << p.GetBidOfferSpread() << std::endl;
	output.close();
}

void GUIConnector::Subscribe() {}

GUIConnector::setGUIService(GUIService* gui_service_) {
	gui_service = gui_service_;
}

#endif
