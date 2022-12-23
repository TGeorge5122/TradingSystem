/**
 * bondpositionservice.hpp
 */
#ifndef BOND_RISK_SERVICE_HPP
#define BOND_RISK_SERVICE_HPP

#include "riskservice.hpp"
#include "products.hpp"

class BondRiskService : public RiskService<Bond>
{
private:
	
	vector<string> product_vec;
	vector<PV01<Bond> > pv_vec;
	vector<ServiceListener<PV01<Bond> >* > listeners;

public:;

	// Get data on our service given a key
	PV01<Bond> GetData(string);

	// The callback that a Connector should invoke for any new or updated data
	void OnMessage(PV01<Bond>&);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	void AddListener(ServiceListener<PV01<Bond> >*);

	// Get all listeners on the Service.
	const vector<ServiceListener<PV01<Bond> >* >& GetListeners() const;

	// Add a position that the service will risk
	void AddPosition(Position<Bond>& position);

	// Get the bucketed risk for the bucket sector
	const PV01< BucketedSector<Bond> > GetBucketedRisk(const BucketedSector<Bond>& sector) const;

};

// Get data on our service given a key
PV01<Bond> BondRiskService::GetData(string product_id) {
	int index = find(product_vec.begin(), product_vec.end(), product_id) - product_vec.begin();
	return pv_vec[index];
}

// The callback that a Connector should invoke for any new or updated data
void BondRiskService::OnMessage(PV01<Bond>& pv_) {

	int index = find(product_vec.begin(), product_vec.end(), pv_.GetProduct().GetProductId()) - product_vec.begin();

	if (index < product_vec.size()) {
		product_vec.erase(product_vec.begin() + index);
		pv_vec.erase(pv_vec.begin() + index);
	}

	product_vec.push_back(pv_.GetProduct().GetProductId());
	pv_vec.push_back(pv_);


	for (int i = 0; i < listeners.size(); i++) {
		listeners[i]->ProcessAdd(pv_);
	}

}

// Add a listener to the Service for callbacks on add, remove, and update events
// for data to the Service.
void BondRiskService::AddListener(ServiceListener<PV01<Bond> >* listener) {
	listeners.push_back(listener);
}

// Get all listeners on the Service.
const vector<ServiceListener<PV01<Bond> >*>& BondRiskService::GetListeners() const {
	return listeners;
}

// Add a position that the service will risk
void BondRiskService::AddPosition(Position<Bond>& position) {

	int index = find(product_vec.begin(), product_vec.end(), position.GetProduct().GetProductId()) - product_vec.begin();

	if (index >= product_vec.size()) {

		PV01<Bond> pv(position.GetProduct(), 0.025, position.GetAggregatePosition());
		OnMessage(pv);

	}
	else {

		PV01<Bond> pv = pv_vec[index];
		pv.SetQuantity(position.GetAggregatePosition());
		OnMessage(pv);

	}

}

// Get the bucketed risk for the bucket sector
const PV01< BucketedSector<Bond> > BondRiskService::GetBucketedRisk(const BucketedSector<Bond>& sector) const {



	double pv = 0;
	long qty = 0;

	std::vector<Bond> products = sector.GetProducts();
	int index;

	for (int i = 0; i < products.size(); i++) {
		int index = find(product_vec.begin(), product_vec.end(), products[i].GetProductId()) - product_vec.begin();
		pv += pv_vec[index].GetPV01() * pv_vec[index].GetQuantity();
		qty += pv_vec[index].GetQuantity();
	
	}

	return PV01<BucketedSector<Bond> >(sector, pv, qty);

}


#endif
