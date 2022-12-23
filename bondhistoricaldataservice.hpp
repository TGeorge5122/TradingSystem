/**
 * bondexecutionservice.hpp
 */
#ifndef BOND_HISTORICAL_DATA_SERVICE_HPP
#define BOND_HISTORICAL_DATA_SERVICE_HPP

#include <fstream>
#include "historicaldataservice.hpp"
#include "bondpositionservice.hpp"
#include "bondriskservice.hpp"
#include "bondinquiryservice.hpp"
#include "bondexecutionservice.hpp"

template <typename T>
class BondHistoricalDataConnector;

template <typename T>
class BondHistoricalDataService : public HistoricalDataService<T>
{
private:
	
	vector<string> product_vec;
	vector<T > data_vec;
	vector<ServiceListener<T>* > listeners;
	BondHistoricalDataConnector<T>* connector;

public:

	BondHistoricalDataService(BondHistoricalDataConnector<T>*);

	// Get data on our service given a key
	T GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(T&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<T >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<T >* >& GetListeners() const;

	// Persist data to a store
	void PersistData(string persistKey, T& data);

};

template <typename T>
class ServiceListenerToHistorical : public ServiceListener<T>
{
private:
	BondHistoricalDataService<T>* service;

public:

	ServiceListenerToHistorical(BondHistoricalDataService<T>*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(T& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(T& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(T& data);

};

template <typename T>
class BondHistoricalDataConnector : public Connector<T >
{

private:

	BondHistoricalDataService<T>* service;

	void publish_data(ExecutionOrder<Bond>&);
	void publish_data(Inquiry<Bond>&);
	void publish_data(Position<Bond>&);
	void publish_data(PriceStream<Bond>&);
	void publish_data(PV01<Bond>&);

public:

	BondHistoricalDataConnector();

	BondHistoricalDataConnector(BondHistoricalDataService<T>*);

	// Publish data to the Connector
	void Publish(T&);

	void Subscribe();

	void setBondService(BondHistoricalDataService<T>*);

};


template <typename T>
BondHistoricalDataService<T>::BondHistoricalDataService(BondHistoricalDataConnector<T>* connector_) {
	connector = connector_;
}

template <typename T>
T BondHistoricalDataService<T>::GetData(string id) {
	int index = find(product_vec.begin(), product_vec.end(), id) - product_vec.begin();
	return data_vec[index];
}

template <typename T>
void BondHistoricalDataService<T>::OnMessage(T& data) {

	int index = find(product_vec.begin(), product_vec.end(), data.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		data_vec.erase(data_vec.begin() + index);
	}

	product_vec.push_back(data.GetProduct().GetProductId());
	data_vec.push_back(data);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(data);
	}

}

template <typename T>
void BondHistoricalDataService<T>::AddListener(ServiceListener<T >* listener) {
	listeners.push_back(listener);
}

template <typename T>
const vector<ServiceListener<T >*>& BondHistoricalDataService<T>::GetListeners() const {
	return listeners;
}

template <typename T>
void BondHistoricalDataService<T>::PersistData(string persistKey, T& data) {
	connector->Publish(data);
}

template <typename T>
ServiceListenerToHistorical<T>::ServiceListenerToHistorical(BondHistoricalDataService<T>* service_) {
	service = service_;
}

template <typename T>
void ServiceListenerToHistorical<T>::ProcessAdd(T& data) {
	service->PersistData("", data);
}

template <typename T>
void ServiceListenerToHistorical<T>::ProcessRemove(T& data) {}

template <typename T>
void ServiceListenerToHistorical<T>::ProcessUpdate(T& data) {}


template <typename T>
BondHistoricalDataConnector<T>::BondHistoricalDataConnector() {

	std::remove("historical_executions.txt");
	std::remove("historical_inquiries.txt"); 
	std::remove("historical_positions.txt");
	std::remove("historical_streaming.txt");
	std::remove("historical_risk.txt");

}

template <typename T>
BondHistoricalDataConnector<T>::BondHistoricalDataConnector(BondHistoricalDataService<T>* service_) {
	service = service_;
}

template <typename T>
void BondHistoricalDataConnector<T>::Publish(T& data) {
	publish_data(data);
}

template <typename T>
void BondHistoricalDataConnector<T>::Subscribe() {}

template <typename T>
void BondHistoricalDataConnector<T>::setBondService(BondHistoricalDataService<T>* service_) {
	service = service_;
}

template <typename T>
void BondHistoricalDataConnector<T>::publish_data(ExecutionOrder<Bond>& data) {
	
	std::ofstream output;
	output.open("historical_executions.txt", ios::app);

	std::string side = data.GetSide() == BID ? "BID" : "OFFER";
	std::string child = data.IsChildOrder() ? "true" : "false";

	output << data.GetProduct().GetProductId() << ",";
	output << data.GetOrderId() << ",";
	output << "Market,";
	output << side << ",";
	output << data.GetPrice() << ",";
	output << data.GetVisibleQuantity() << ",";
	output << data.GetHiddenQuantity() << ",";
	output << data.GetParentOrderId() << ",";
	output << child;

	output << std::endl;
	output.close();
}

template <typename T>
void BondHistoricalDataConnector<T>::publish_data(Inquiry<Bond>& data) {

	std::string side = data.GetSide() == BUY ? "BUY" : "SELL";
	InquiryState istate = data.GetState();
	std::string state;

	if (istate == RECEIVED) {
		state = "RECEIVED";
	}
	else if (istate == QUOTED) {
		state = "QUOTED";
	}
	else if (istate == DONE) {
		state = "DONE";
	}
	else if (istate == REJECTED) {
		state = "REJECTED";
	}
	else {
		state = "CUSTOMER_REJECTED";
	}

	std::ofstream output;
	output.open("historical_inquiries.txt", ios::app);

	output << data.GetInquiryId() << ",";
	output << data.GetProduct().GetProductId() << ",";
	output << side << ",";
	output << data.GetQuantity() << ",";
	output << data.GetPrice() << ",";
	output << state;

	output << std::endl;
	output.close();
}

template <typename T>
void BondHistoricalDataConnector<T>::publish_data(Position<Bond>& data) {

	std::string book_one("TRSY1");
	std::string book_two("TRSY2");
	std::string book_three("TRSY3");

	std::ofstream output;
	output.open("historical_positions.txt", ios::app);

	output << data.GetProduct().GetProductId() << ",";
	output << data.GetPosition(book_one) << ",";
	output << data.GetPosition(book_two) << ",";
	output << data.GetPosition(book_three) << ",";
	output << data.GetAggregatePosition();

	output << std::endl;
	output.close();
}

template <typename T>
void BondHistoricalDataConnector<T>::publish_data(PriceStream<Bond>& data) {

	PriceStreamOrder bid = data.GetBidOrder();
	PriceStreamOrder offer = data.GetOfferOrder();

	std::ofstream output;
	output.open("historical_streaming.txt", ios::app);

	output << data.GetProduct() << ",";
	output << "BID,";
	output << bid.GetPrice() << ",";
	output << bid.GetVisibleQuantity() << ",";
	output << bid.GetHiddenQuantity() << ",";
	output << "OFFER,";
	output << offer.GetPrice() << ",";
	output << offer.GetVisibleQuantity() << ",";
	output << offer.GetHiddenQuantity() << ",";

	output << std::endl;
	output.close();
}

template <typename T>
void BondHistoricalDataConnector<T>::publish_data(PV01<Bond>& data) {

	std::ofstream output;
	output.open("historical_risk.txt", ios::app);

	output << data.GetProduct().GetProductId() << ",";
	output << data.GetPV01() << ",";
	output << data.GetQuantity();

	output << std::endl;
	output.close();
}

#endif
