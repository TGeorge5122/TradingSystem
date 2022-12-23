/**
 * bondexecutionservice.hpp
 */
#ifndef BOND_STREAMING_SERVICE_HPP
#define BOND_STREAMING_SERVICE_HPP

#include "streamingservice.hpp"
#include "bondalgostreamingservice.hpp"

 //Forward declaration for use in BondExecutionService
class BondStreamingConnector;

class BondStreamingService : public StreamingService<Bond>
{
private:
	
	vector<string> product_vec;
	vector<PriceStream<Bond> > stream_vec;
	vector<ServiceListener<PriceStream<Bond> >* > listeners;
	BondStreamingConnector* stream_connector;

public:

	BondStreamingService(BondStreamingConnector*);

	// Get data on our service given a key
	PriceStream<Bond> GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(PriceStream<Bond>&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<PriceStream<Bond> >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<PriceStream<Bond> >* >& GetListeners() const;

	// Publish two-way prices
	void PublishPrice(const PriceStream<Bond>& priceStream);

};

class BondAlgoStreamingServiceListener: public ServiceListener<AlgoStream<Bond> >
{
private:
	BondStreamingService* stream_service;

public:

	BondAlgoStreamingServiceListener(BondStreamingService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(AlgoStream<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(AlgoStream<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(AlgoStream<Bond>& data);

};

// Connector subscribing data from marketdata.txt to BondMarketDataService.
class BondStreamingConnector : public Connector<PriceStream<Bond> >
{

private:

	BondStreamingService* stream_service;

public:

	BondStreamingConnector();

	BondStreamingConnector(BondStreamingService*);

	// Publish data to the Connector
	void Publish(PriceStream<Bond>&);

	void Subscribe();

	setBondStreamingService(BondStreamingService*);

};

BondStreamingService::BondStreamingService(BondStreamingConnector* stream_connector_) {
	stream_connector = stream_connector_;
}

// Get data on our service given a key
PriceStream<Bond> BondStreamingService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return stream_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondStreamingService::OnMessage(PriceStream <Bond>& stream) {
	
	int index = find(product_vec.begin(), product_vec.end(), stream.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		stream_vec.erase(stream_vec.begin() + index);
	}

	product_vec.push_back(stream.GetProduct().GetProductId());
	stream_vec.push_back(stream);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(stream);
	}

}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondStreamingService::AddListener(ServiceListener<PriceStream<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<PriceStream<Bond> >*>& BondStreamingService::GetListeners() const {
	return listeners;
}

// Publish two-way prices
void BondStreamingService::PublishPrice(const PriceStream<Bond>& priceStream) {
	PriceStream<Bond> ps = priceStream;
	OnMessage(ps);
	stream_connector->Publish(ps);
}

BondAlgoStreamingServiceListener::BondAlgoStreamingServiceListener(BondStreamingService* stream_service_) {
	stream_service = stream_service_;
}

// Listener callback to process an add event to the Service
void BondAlgoStreamingServiceListener::ProcessAdd(AlgoStream<Bond>& data) {
	stream_service->PublishPrice(data.GetPriceStream());
}

// Listener callback to process a remove event to the Service
void BondAlgoStreamingServiceListener::ProcessRemove(AlgoStream<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondAlgoStreamingServiceListener::ProcessUpdate(AlgoStream<Bond>& data) {}

BondStreamingConnector::BondStreamingConnector() {}

BondStreamingConnector::BondStreamingConnector(BondStreamingService* stream_service_) {
	stream_service = stream_service_;
}

// Publish data to the Connector
void BondStreamingConnector::Publish(PriceStream<Bond>& ps) {

	std::cout << "Bond: " << ps.GetProduct().GetProductId() << ", ";
	std::cout << "Bid: " << ps.GetBidOrder().GetPrice() << ", ";
	std::cout << "Bid Quantity: " << ps.GetBidOrder().GetVisibleQuantity() + ps.GetBidOrder().GetHiddenQuantity() << ", ";
	std::cout << "Offer: " << ps.GetOfferOrder().GetPrice() << ", ";
	std::cout << "Offer Quantity: " << ps.GetOfferOrder().GetVisibleQuantity() + ps.GetOfferOrder().GetHiddenQuantity() << std::endl;
}

void BondStreamingConnector::Subscribe() {}

BondStreamingConnector::setBondStreamingService(BondStreamingService* stream_service_) {
	stream_service = stream_service_;
}

#endif
