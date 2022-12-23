/**
 * bonduniverseservice.hpp
 */
#ifndef BOND_UNIVERSE_SERVICE_HPP
#define BOND_UNIVERSE_SERVICE_HPP

#include <iostream>
#include <algorithm>
#include "products.hpp"
#include "soa.hpp"

class BondUniverseService : public Service<string, Bond> {

public:

	Bond GetData(string);

	vector<Bond> GetUniverse();

	void OnMessage(Bond& bond);

	void AddListener(ServiceListener<Bond >* listener);

	const vector<ServiceListener<Bond>*>& GetListeners() const;

private:

	std::vector<string> id_vec;
	std::vector<Bond> bond_universe;

};

Bond BondUniverseService::GetData(string id) {
	int index = find(id_vec.begin(), id_vec.end(), id) - id_vec.begin();
	return bond_universe[index];
}

vector<Bond> BondUniverseService::GetUniverse() {
	return bond_universe;
}

void BondUniverseService::OnMessage(Bond& bond) {
	id_vec.push_back(bond.GetProductId());
	bond_universe.push_back(bond);
}

void BondUniverseService::AddListener(ServiceListener<Bond >* listener) {}

const vector<ServiceListener<Bond>*>& BondUniverseService::GetListeners() const {}

#endif