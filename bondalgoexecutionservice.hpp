/**
 * bondalgoexecutionservice.hpp
 */
#ifndef BOND_ALGO_EXECUTION_SERVICE_HPP
#define BOND_ALGO_EXECUTION_SERVICE_HPP

#include "executionservice.hpp"
#include "bondmarketdataservice.hpp"

template<typename T>
class AlgoExecution {
private:

	ExecutionOrder<T> exeorder;
	Market mkt;

public:

	AlgoExecution(ExecutionOrder<T>, Market);

	ExecutionOrder<T> GetExecutionOrder();

	Market GetMarket();

};

template<typename T>
AlgoExecution<T>::AlgoExecution(ExecutionOrder<T> exeorder_, Market mkt_) {
	exeorder = exeorder_;
	mkt = mkt_;
}

template<typename T>
ExecutionOrder<T> AlgoExecution<T>::GetExecutionOrder() {
	return exeorder;
}

template<typename T>
Market AlgoExecution<T>::GetMarket() {
	return mkt;
}

//Not an ExecutionService since we would need to set : public ExecutionService<AlgoExecution<Bond> > but AlgoExecution is not an ExecutionOrder
class BondAlgoExecutionService
{
private:
	
	vector<string> product_vec;
	vector<AlgoExecution<Bond> > algo_exe_vec;
	vector<ServiceListener<AlgoExecution<Bond> >* > listeners;

public:;

	// Get data on our service given a key
	  AlgoExecution<Bond> GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(AlgoExecution<Bond>&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<AlgoExecution<Bond> >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<AlgoExecution<Bond> >* >& GetListeners() const;

	// Execute an order on a market
	void ExecuteOrder(const ExecutionOrder<Bond>& order, Market market);

};

class BondMarketDataServiceListener : public ServiceListener<OrderBook<Bond> >
{
private:
	BondAlgoExecutionService* algoexe_service;
	PricingSide side;
	Market mkt;

public:

	BondMarketDataServiceListener(BondAlgoExecutionService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(OrderBook<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(OrderBook<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(OrderBook<Bond>& data);

};

// Get data on our service given a key
AlgoExecution<Bond> BondAlgoExecutionService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return algo_exe_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondAlgoExecutionService::OnMessage(AlgoExecution<Bond>& exe) {
	
	int index = find(product_vec.begin(), product_vec.end(), exe.GetExecutionOrder().GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		algo_exe_vec.erase(algo_exe_vec.begin() + index);
	}

	product_vec.push_back(exe.GetExecutionOrder().GetProduct().GetProductId());
	algo_exe_vec.push_back(exe);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(exe);
	}

}

// Execute an order on a market
void BondAlgoExecutionService::ExecuteOrder(const ExecutionOrder<Bond>& order, Market market) {
	AlgoExecution<Bond> algo_exe(order, market);
	OnMessage(algo_exe);
}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondAlgoExecutionService::AddListener(ServiceListener<AlgoExecution<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<AlgoExecution<Bond> >*>& BondAlgoExecutionService::GetListeners() const {
	return listeners;
}

BondMarketDataServiceListener::BondMarketDataServiceListener(BondAlgoExecutionService* algoexe_service_) {
	side = BID;
	algoexe_service = algoexe_service_;
	mkt = BROKERTEC;
}

// Listener callback to process an add event to the Service
void BondMarketDataServiceListener::ProcessAdd(OrderBook<Bond>& data) {

	vector<Order> bid_stack = data.GetBidStack();
	vector<Order> offer_stack = data.GetOfferStack();

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

	if (abs(best_offer.GetPrice() - best_bid.GetPrice() < (1.0 / 128) + 1e-9)) {

		if (side == BID) {

			//No access to std::to_string - setting constant OrderID
			ExecutionOrder<Bond> exec(data.GetProduct(), side, "OrderID", MARKET, best_offer.GetPrice(), best_offer.GetQuantity(), 0, "", false);
			algoexe_service->ExecuteOrder(exec, mkt);
			side = OFFER;
		}
		else {
			ExecutionOrder<Bond> exec(data.GetProduct(), side, "OrderID", MARKET, best_bid.GetPrice(), best_bid.GetQuantity(), 0, "", false);
			algoexe_service->ExecuteOrder(exec, mkt);
			side = BID;
		}

		if (mkt == BROKERTEC) {
			mkt = ESPEED;
		}
		else if (mkt == ESPEED) {
			mkt = CME;
		}
		else {
			mkt = BROKERTEC;
		}
	}
}

// Listener callback to process a remove event to the Service
void BondMarketDataServiceListener::ProcessRemove(OrderBook<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondMarketDataServiceListener::ProcessUpdate(OrderBook<Bond>& data) {}

#endif
