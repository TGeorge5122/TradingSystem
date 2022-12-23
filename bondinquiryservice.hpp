/**
 * bondinquiryservice.hpp
 */
#ifndef BOND_INQUIRY_SERVICE_HPP
#define BOND_INQUIRY_SERVICE_HPP

#include "treasuryprices.hpp"
#include "inquiryservice.hpp"
#include "bonduniverseservice.hpp"
#include "util.hpp"

//Forward declaration for use in BondInquiryService
class BondInquiryConnector;

class BondInquiryService : public InquiryService<Bond>
{
private:

	vector<string> product_vec;
	vector<string> inquiry_id_vec;
	vector<Inquiry<Bond> > inq_vec;
	vector<ServiceListener<Inquiry<Bond> >* > listeners;
	BondInquiryConnector* bic;

public:

	BondInquiryService(BondInquiryConnector*);

	// Get data on our service given a key
	Inquiry<Bond> GetData(string product_id);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(Inquiry<Bond>& ob);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<Inquiry<Bond> >* listener);

	// Get all listeners on the Service.
	const vector<ServiceListener<Inquiry<Bond> >* >& GetListeners() const;

	// Send a quote back to the client
	void SendQuote(const string& inquiryId, double price);

	// Reject an inquiry from the client
	void RejectInquiry(const string& inquiryId);

};

class BondInquiryServiceListener : public ServiceListener<Inquiry<Bond> > 
{
private:
	BondInquiryService* service;

public:

	BondInquiryServiceListener(BondInquiryService*);

	// Listener callback to process an add event to the Service
	void ProcessAdd(Inquiry<Bond>& data);

	// Listener callback to process a remove event to the Service
	void ProcessRemove(Inquiry<Bond>& data);

	// Listener callback to process an update event to the Service
	void ProcessUpdate(Inquiry<Bond>& data);

};

// Connector subscribing data from marketdata.txt to BondMarketDataService.
class BondInquiryConnector : public Connector<Inquiry<Bond> >
{

private:

	BondInquiryService* inq_service;
	BondUniverseService* uni_service;

public:

	BondInquiryConnector();

	BondInquiryConnector(BondInquiryService*, BondUniverseService*);

	// Publish data to the Connector
	void Publish(Inquiry<Bond>&);

	// Subscribe to prices.txt
	void Subscribe();

	setBondInquiryService(BondInquiryService*);
	setBondUniverseService(BondUniverseService*);

};

BondInquiryService::BondInquiryService(BondInquiryConnector* bic_) {
	bic = bic_;
}

// Get data on our service given a key
Inquiry<Bond> BondInquiryService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return inq_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondInquiryService::OnMessage(Inquiry<Bond>& inquiry) {

	int index = find(product_vec.begin(), product_vec.end(), inquiry.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		inq_vec.erase(inq_vec.begin() + index);
		inquiry_id_vec.erase(inquiry_id_vec.begin() + index);

	}

	if (inquiry.GetState() == QUOTED) {
		inquiry.SetState(DONE);
	}

	product_vec.push_back(inquiry.GetProduct().GetProductId());
	inq_vec.push_back(inquiry);
	inquiry_id_vec.push_back(inquiry.GetInquiryId());


	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(inquiry);
	}

}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondInquiryService::AddListener(ServiceListener<Inquiry<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<Inquiry<Bond> >*>& BondInquiryService::GetListeners() const {
	return listeners;
}

// Send a quote back to the client
void BondInquiryService::SendQuote(const string& inquiryId, double price) {
	
	int index = find(inquiry_id_vec.begin(), inquiry_id_vec.end(), inquiryId) - inquiry_id_vec.begin();
	
	Inquiry<Bond> inquiry = inq_vec[index];
	
	inquiry.SetPrice(price);
	
	bic->Publish(inquiry);

}

// Reject an inquiry from the client
void BondInquiryService::RejectInquiry(const string& inquiryId) {

	int index = find(inquiry_id_vec.begin(), inquiry_id_vec.end(), inquiryId) - inquiry_id_vec.begin();
	inq_vec[index].SetState(REJECTED);
}

BondInquiryServiceListener::BondInquiryServiceListener(BondInquiryService* service_) {
	service = service_;
}

// Listener callback to process an add event to the Service
void BondInquiryServiceListener::ProcessAdd(Inquiry<Bond>& data) {
	if (data.GetState() == RECEIVED) {
		service->SendQuote(data.GetInquiryId(), 100.0);
	}

}

// Listener callback to process a remove event to the Service
void BondInquiryServiceListener::ProcessRemove(Inquiry<Bond>& data) {}

// Listener callback to process an update event to the Service
void BondInquiryServiceListener::ProcessUpdate(Inquiry<Bond>& data) {}

BondInquiryConnector::BondInquiryConnector() {}

BondInquiryConnector::BondInquiryConnector(BondInquiryService* inq_service_, BondUniverseService* uni_service_) {
	inq_service = inq_service_;
	uni_service = uni_service_;
}

BondInquiryConnector::setBondInquiryService(BondInquiryService* inq_service_) {
	inq_service = inq_service_;
}
BondInquiryConnector::setBondUniverseService(BondUniverseService* uni_service_) {
	uni_service = uni_service_;
}

void BondInquiryConnector::Publish(Inquiry<Bond>& data) {
	data.SetState(QUOTED);
	inq_service->OnMessage(data);
}

void BondInquiryConnector::Subscribe() {

	ifstream input_file;
	input_file.open("inquiries.txt");
	string update;

	Bond b;
	TreasuryPrices price;
	long quantity;
	InquiryState state;

	while (getline(input_file, update)) {

		vector<string> update_split = split(update, ",");

		b = uni_service->GetData(update_split[1]);
		std::sscanf(update_split[3].c_str(), "%d", &quantity);
		price = TreasuryPrices(update_split[4]);

		if (update_split[5] == "RECEIVED") {
			state= RECEIVED;
		}
		else if (update_split[5] == "QUOTED") {
			state = QUOTED;
		}
		else if (update_split[5] == "DONE") {
			state = DONE;
		}
		else if (update_split[5] == "REJECTED") {
			state = REJECTED;
		}
		else{
			state = CUSTOMER_REJECTED;
		}

		Inquiry<Bond> t(update_split[0], b, update_split[2] == "BUY" ? BUY : SELL, quantity, price.toDouble(), state);
		inq_service->OnMessage(t);
	}
}

#endif
