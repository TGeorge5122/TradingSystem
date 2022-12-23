/**
 * bondexecutionservice.hpp
 */
#ifndef BOND_EXECUTION_SERVICE_HPP
#define BOND_EXECUTION_SERVICE_HPP

#include "executionservice.hpp"
#include "bondalgoexecutionservice.hpp"
#include "bondtradebookingservice.hpp"

 //Forward declaration for use in BondExecutionService
class BondExecutionConnector;

class BondExecutionService : public ExecutionService<Bond>
{
private:
	
	vector<string> product_vec;
	vector<ExecutionOrder<Bond> > exe_vec;
	vector<ServiceListener<ExecutionOrder<Bond> >* > listeners;
	BondExecutionConnector* exe_connector;

public:

	BondExecutionService(BondExecutionConnector*);

	// Get data on our service given a key
	ExecutionOrder<Bond> GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(ExecutionOrder<Bond>&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<ExecutionOrder<Bond> >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<ExecutionOrder<Bond> >* >& GetListeners() const;

	// Execute an order on a market
	void ExecuteOrder(const ExecutionOrder<Bond>& order, Market market);

};

class BondAlgoExecutionServiceListener: public ServiceListener<AlgoExecution<Bond> >
{
private:
	BondExecutionService* exe_service;

public:

	BondAlgoExecutionServiceListener(BondExecutionService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(AlgoExecution<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(AlgoExecution<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(AlgoExecution<Bond>& data);

};

class BondExecutionServiceListener : public ServiceListener<ExecutionOrder<Bond> >
{
private:
	BondTradeBookingService* btb_service;
	std::string book;

public:

	BondExecutionServiceListener(BondTradeBookingService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(ExecutionOrder<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(ExecutionOrder<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(ExecutionOrder<Bond>& data);

};

// Connector subscribing data from marketdata.txt to BondMarketDataService.
class BondExecutionConnector : public Connector<ExecutionOrder<Bond> >
{

private:

	BondExecutionService* exe_service;

public:

	BondExecutionConnector();

	BondExecutionConnector(BondExecutionService*);

	// Publish data to the Connector
	void Publish(ExecutionOrder<Bond>&);

	void Subscribe();

	setBondExecutionService(BondExecutionService*);

};

BondExecutionService::BondExecutionService(BondExecutionConnector* exe_connector_) {
	exe_connector = exe_connector_;
}

// Get data on our service given a key
ExecutionOrder<Bond> BondExecutionService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return exe_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondExecutionService::OnMessage(ExecutionOrder<Bond>& exe) {
	
	int index = find(product_vec.begin(), product_vec.end(), exe.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		exe_vec.erase(exe_vec.begin() + index);
	}

	product_vec.push_back(exe.GetProduct().GetProductId());
	exe_vec.push_back(exe);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(exe);
	}

}

// Execute an order on a market
void BondExecutionService::ExecuteOrder(const ExecutionOrder<Bond>& order, Market market) {
	ExecutionOrder<Bond> ord = order;
	OnMessage(ord);
	exe_connector->Publish(ord);
}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondExecutionService::AddListener(ServiceListener<ExecutionOrder<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<ExecutionOrder<Bond> >*>& BondExecutionService::GetListeners() const {
	return listeners;
}

BondAlgoExecutionServiceListener::BondAlgoExecutionServiceListener(BondExecutionService* exe_service_) {
	exe_service = exe_service_;
}

// Listener callback to process an add event to the Service
void BondAlgoExecutionServiceListener::ProcessAdd(AlgoExecution<Bond>& data) {
	exe_service->ExecuteOrder(data.GetExecutionOrder(), data.GetMarket());
}

// Listener callback to process a remove event to the Service
void BondAlgoExecutionServiceListener::ProcessRemove(AlgoExecution<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondAlgoExecutionServiceListener::ProcessUpdate(AlgoExecution<Bond>& data) {}


BondExecutionServiceListener::BondExecutionServiceListener(BondTradeBookingService* btb_service_) {
	btb_service = btb_service_;
	book = "TRSY1";
}

// Listener callback to process an add event to the Service
void BondExecutionServiceListener::ProcessAdd(ExecutionOrder<Bond>& data) {

	//No access to std::to_string - setting constant trade id
	Trade<Bond> t(data.GetProduct(), "Trade_ID", data.GetPrice(), book, data.GetVisibleQuantity() + data.GetHiddenQuantity(), data.GetSide() == BID ? BUY : SELL);

	btb_service->BookTrade(t);

	if (book == "TRSY1") {
		book = "TRSY2";
	}
	else if (book == "TRSY2") {
		book = "TRSY3";
	}
	else{
		book = "TRSY1";
	}

}

// Listener callback to process a remove event to the Service
void BondExecutionServiceListener::ProcessRemove(ExecutionOrder<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondExecutionServiceListener::ProcessUpdate(ExecutionOrder<Bond>& data) {}

BondExecutionConnector::BondExecutionConnector() {}

BondExecutionConnector::BondExecutionConnector(BondExecutionService* exe_service_) {
	exe_service = exe_service_;
}

// Publish data to the Connector
void BondExecutionConnector::Publish(ExecutionOrder<Bond>& ord) {

	std::string side = ord.GetSide() == BID ? "BID" : "OFFER";

	std::cout << "Executing Order: " << std::endl;

	std::cout << "Bond: " << ord.GetProduct().GetProductId() << ", ";
	std::cout << "OrderID: " << ord.GetOrderId() << ", ";
	std::cout << "OrderType: Market, ";
	std::cout << "OrderSide: " << side << ", ";
	std::cout << "Price: " << ord.GetPrice() << ", ";
	std::cout << "Quantiy: " << ord.GetVisibleQuantity() + ord.GetHiddenQuantity() << std::endl;

	std::cout << "Order Executed" << std::endl;
}

void BondExecutionConnector::Subscribe() {}

BondExecutionConnector::setBondExecutionService(BondExecutionService* exe_service_) {
	exe_service = exe_service_;
}

#endif
