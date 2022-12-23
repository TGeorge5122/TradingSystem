/**
 * bondpositionservice.hpp
 */
#ifndef BOND_POSITION_SERVICE_HPP
#define BOND_POSITION_SERVICE_HPP

#include "positionservice.hpp"
#include "bondtradebookingservice.hpp"
#include "bondriskservice.hpp"

class BondPositionService : public PositionService<Bond>
{
private:
	
	vector<string> product_vec;
	vector<Position<Bond> > position_vec;
	vector<ServiceListener<Position<Bond> >* > listeners;

public:;

	// Get data on our service given a key
	Position<Bond> GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Position<Bond>&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<Position<Bond> >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<Position<Bond> >* >& GetListeners() const;

	// Add a trade to the service
	void AddTrade(const Trade<Bond>&);

};

class BondTradeBookingServiceListener : public ServiceListener<Trade<Bond> >
{
private:
	BondPositionService* position_service;

public:

	BondTradeBookingServiceListener(BondPositionService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Trade<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Trade<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Trade<Bond>& data);

};

class BondPositionServiceListener : public ServiceListener<Position<Bond> >
{
private:
	BondRiskService* risk_service;

public:

	BondPositionServiceListener(BondRiskService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Position<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Position<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Position<Bond>& data);

};

// Get data on our service given a key
Position<Bond> BondPositionService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return position_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondPositionService::OnMessage(Position<Bond>& pos) {

	int index = find(product_vec.begin(), product_vec.end(), pos.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		position_vec.erase(position_vec.begin() + index);
	}

	product_vec.push_back(pos.GetProduct().GetProductId());
	position_vec.push_back(pos);


	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(pos);
	}

}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondPositionService::AddListener(ServiceListener<Position<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<Position<Bond> >*>& BondPositionService::GetListeners() const {
	return listeners;
}

// Add a trade to the service
void BondPositionService::AddTrade(const Trade<Bond>& trade) {

	int index = find(product_vec.begin(), product_vec.end(), trade.GetProduct().GetProductId()) - product_vec.begin();

	std::string book = trade.GetBook();

	//Product Id does not yet exist, need to start from empty position
	if (index >= product_vec.size()) {
		
		Position<Bond> p(trade.GetProduct());
		p.AddQty(book, trade.GetSide() == BUY ? trade.GetQuantity() : -trade.GetQuantity());
		OnMessage(p);

	}
	//Product id exists, position exists
	else {

		Position<Bond> p = position_vec[index];
		p.AddQty(book, trade.GetSide() == BUY ? trade.GetQuantity() : -trade.GetQuantity());
		OnMessage(p);

	}

}

BondTradeBookingServiceListener::BondTradeBookingServiceListener(BondPositionService* pos_service_) {
	position_service = pos_service_;
}

// Listener callback to process an add event to the Service
void BondTradeBookingServiceListener::ProcessAdd(Trade<Bond>& data) {
	position_service->AddTrade(data);
}

// Listener callback to process a remove event to the Service
void BondTradeBookingServiceListener::ProcessRemove(Trade<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondTradeBookingServiceListener::ProcessUpdate(Trade<Bond>& data) {}

BondPositionServiceListener::BondPositionServiceListener(BondRiskService* risk_service_) {
	risk_service = risk_service_;
}

// Listener callback to process an add event to the Service
void BondPositionServiceListener::ProcessAdd(Position<Bond>& data) {
	risk_service->AddPosition(data);
}

// Listener callback to process a remove event to the Service
void BondPositionServiceListener::ProcessRemove(Position<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondPositionServiceListener::ProcessUpdate(Position<Bond>& data) {}

#endif
