/**
 * bondalgoexecutionservice.hpp
 */
#ifndef BOND_ALGO_STREAMING_SERVICE_HPP
#define BOND_ALGO_STREAMING_SERVICE_HPP

#include "streamingservice.hpp"
#include "bondpricingservice.hpp"

template<typename T>
class AlgoStream {
private:

	PriceStream<T> ps;

public:

	AlgoStream(PriceStream<T>);

	PriceStream<T> GetPriceStream();

};

template<typename T>
AlgoStream<T>::AlgoStream(PriceStream<T> ps_) {
	ps = ps_;
}

template<typename T>
PriceStream<T> AlgoStream<T>::GetPriceStream() {
	return ps;
}

//Not a StreamingService since we would need to set : public ExecutionService<AlgoStream<Bond> > but AlgoStream is not a PriceStream
class BondAlgoStreamingService
{
private:
	
	vector<string> product_vec;
	vector<AlgoStream<Bond> > algo_stream_vec;
	vector<ServiceListener<AlgoStream<Bond> >* > listeners;

public:;

	// Get data on our service given a key
	AlgoStream<Bond> GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(AlgoStream<Bond>&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<AlgoStream<Bond> >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<AlgoStream<Bond> >* >& GetListeners() const;

	// Publish two-way prices
	void PublishPrice(const PriceStream<Bond>& priceStream);

};

class BondPricingServiceListener : public ServiceListener<Price<Bond> >
{
private:
	BondAlgoStreamingService* algostream_service;
	long vis_qty;

public:

	BondPricingServiceListener(BondAlgoStreamingService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Price<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Price<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Price<Bond>& data);

};

// Get data on our service given a key
AlgoStream<Bond> BondAlgoStreamingService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return algo_stream_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondAlgoStreamingService::OnMessage(AlgoStream<Bond>& stream) {
	
	int index = find(product_vec.begin(), product_vec.end(), stream.GetPriceStream().GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		algo_stream_vec.erase(algo_stream_vec.begin() + index);
	}

	product_vec.push_back(stream.GetPriceStream().GetProduct().GetProductId());
	algo_stream_vec.push_back(stream);

	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(stream);
	}

}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondAlgoStreamingService::AddListener(ServiceListener<AlgoStream<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<AlgoStream<Bond> >*>& BondAlgoStreamingService::GetListeners() const {
	return listeners;
}

// Publish two-way prices
void BondAlgoStreamingService::PublishPrice(const PriceStream<Bond>& priceStream) {
	AlgoStream<Bond> as(priceStream);
	OnMessage(as);
}

BondPricingServiceListener::BondPricingServiceListener(BondAlgoStreamingService* algostream_service_) {
	algostream_service = algostream_service_;
	vis_qty = 1000000;
}

// Listener callback to process an add event to the Service
void BondPricingServiceListener::ProcessAdd(Price<Bond>& data) {

	double spread = data.GetBidOfferSpread();
	double mid = data.GetMid();

	Bond b = data.GetProduct();
	PriceStreamOrder bid(mid - spread / 2, vis_qty, vis_qty * 2, BID);
	PriceStreamOrder offer(mid + spread / 2, vis_qty, vis_qty * 2, OFFER);

	if (vis_qty == 1000000) {
		vis_qty = 2000000;
	}
	else {
		vis_qty = 1000000;
	}

	PriceStream<Bond> ps(b, bid, offer);
	algostream_service->PublishPrice(ps);

}

// Listener callback to process a remove event to the Service
void BondPricingServiceListener::ProcessRemove(Price<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondPricingServiceListener::ProcessUpdate(Price<Bond>& data) {}

#endif
