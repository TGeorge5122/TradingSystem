/**
 * datagenerator.hpp
 * Generates inquires.txt, marketdata.txt, prices.txt, and trades.txt
 *
 */
#ifndef DATA_GENERATOR_HPP
#define DATA_GENERATOR_HPP

#include <fstream>
#include <vector>
#include <cstdlib>
#include "products.hpp"
#include "treasuryprices.hpp"
#include "tradebookingservice.hpp"

class BondGenerator {
private:
	//Discrete half-spread sizes
	std::vector<TreasuryPrices> halfspread_sizes;
	std::vector<Bond> bonds;
	TreasuryPrices tick;

public:

	BondGenerator(const std::vector<Bond>, const std::vector<TreasuryPrices>);

	void generateInquiries(const int);

	void generateMarketData(const int, const int);

	void generatePrices(const int);

	void generateTrades(const int);
};

BondGenerator::BondGenerator(const std::vector<Bond> bonds_, const std::vector<TreasuryPrices> hs) {
	halfspread_sizes = hs;
	tick = TreasuryPrices(0, 0, 1);
	bonds = bonds_;
}

void BondGenerator::generateInquiries(const int n_inquiries) {

	std::ofstream output;
	output.open("inquiries.txt");

	std::vector<Side> trade_side;

	for (int i = 0; i < bonds.size(); ++i) {
		trade_side.push_back(BUY);
	}

	for (int i = 0; i < n_inquiries; i++) {

		for (int bond_index = 0; bond_index < bonds.size(); bond_index++) {

			output << bond_index + bonds.size() * i << "," << bonds[bond_index].GetProductId();

			if (trade_side[bond_index] == BUY) {
				output << ",BUY";
				trade_side[bond_index] = SELL;
			}
			else{
				output << ",SELL";
				trade_side[bond_index] = BUY;
			}

			//Old way to generate random numbers - biased - but don't have to deal with include<random> and out of date g++ issues
			//Not allowing 101 to make computation easier
			output << "," << (1 + std::rand() % 5) * 1000000;
			output << "," << TreasuryPrices(99 + std::rand() % 2, std::rand() % (32), std::rand() % 8);
			output << ",RECEIVED" << std::endl;

		}

	}

	output.close();

}


//Generate marketdata.txt
void BondGenerator::generateMarketData(const int n_updates, const int book_depth = 5) {


	std::ofstream output;
	output.open("marketdata.txt");

	//Used to oscillate between discrete spread levels
	std::vector<int> spread_level;
	std::vector<int> spread_increment_direction;

	//Tracks each bonds midpoint
	std::vector<TreasuryPrices> midpoints;

	//Tracks whether to increase or decrease the midpoint
	//Could be one value since all bonds have the same oscillation pattern
	std::vector<TreasuryPrices> midpoint_increments;

	for (int i = 0; i < bonds.size(); ++i) {
		midpoints.push_back(TreasuryPrices(99, 0, 0));
		midpoint_increments.push_back(TreasuryPrices(0, 0, -1));
		spread_level.push_back(0);
		spread_increment_direction.push_back(-1);
	}

	//Create n_updates for each bond
	for (int i = 0; i < n_updates; i++) {

		//Create an update for each bond - each update will be on one line with product id, mid, and book_depth number of levels
		for (int bond_index = 0; bond_index < bonds.size(); bond_index++) {

			output << bonds[bond_index].GetProductId() << "," << midpoints[bond_index] << ",MID";

			for (int j = 0; j < book_depth; j++) {

				output << "," << midpoints[bond_index] - halfspread_sizes[spread_level[bond_index]] - tick * j << "," << (j+1) * 10000000 << ",BID";
				output << "," << midpoints[bond_index] + halfspread_sizes[spread_level[bond_index]] + tick * j << "," << (j+1) * 10000000 << ",OFFER";

			}

			output << std::endl;

			//If midpoint is at min or max value switch increment direction
			if (midpoints[bond_index].atMin() || midpoints[bond_index].atMax()) midpoint_increments[bond_index].negate();
			midpoints[bond_index] = midpoints[bond_index] + midpoint_increments[bond_index];

			//If spread size is at min or max level, switch increment direction
			if (spread_level[bond_index] == 0 || spread_level[bond_index] == halfspread_sizes.size() - 1) spread_increment_direction[bond_index] *= -1;
			spread_level[bond_index] += spread_increment_direction[bond_index];
		}

	}

	output.close();
}

//Generate prices.txt
void BondGenerator::generatePrices(const int n_updates) {


	std::ofstream output;
	output.open("prices.txt");

	//Used to oscillate between discrete spread levels
	std::vector<int> spread_level;
	std::vector<int> spread_increment_direction;

	std::vector<TreasuryPrices> bids;
	std::vector<TreasuryPrices> bid_increments;

	for (int i = 0; i < bonds.size(); ++i) {
		bids.push_back(TreasuryPrices(99, 0, 0));
		bid_increments.push_back(TreasuryPrices(0, 0, -1));
		spread_level.push_back(0);
		spread_increment_direction.push_back(-1);
	}

	//Create n_updates for each bond
	for (int i = 0; i < n_updates; i++) {

		for (int bond_index = 0; bond_index < bonds.size(); bond_index++) {

			TreasuryPrices offer = bids[bond_index] + halfspread_sizes[spread_level[bond_index]] * 2;

			while (offer.gtMax()) {
				bids[bond_index] = bids[bond_index] - tick;
				offer = offer - tick;

				bid_increments[bond_index] = TreasuryPrices(0, 0, -1);
			}

			output << bonds[bond_index].GetProductId();
			output << "," << bids[bond_index];
			output << "," << offer;
			output << std::endl;

			if (bids[bond_index].atMin()) bid_increments[bond_index].negate();
			bids[bond_index] = bids[bond_index] + bid_increments[bond_index];

			if (spread_level[bond_index] == 0 || spread_level[bond_index] == halfspread_sizes.size() - 1) spread_increment_direction[bond_index] *= -1;
			spread_level[bond_index] += spread_increment_direction[bond_index];

		}

	}

	output.close();
}

//Generate prices.txt
void BondGenerator::generateTrades(const int n_trades) {


	std::ofstream output;
	output.open("trades.txt");

	std::vector<TreasuryPrices> bids;
	std::vector<TreasuryPrices> offers;
	std::vector<TreasuryPrices> increment;
	
	//Assume passive trades - buys executed at best bid, asks executed at best offer
	std::vector<Side> trade_side;

	for (int i = 0; i < bonds.size(); ++i) {
		bids.push_back(TreasuryPrices(99, 0, 0));
		offers.push_back(TreasuryPrices(100, 0, 0));
		increment.push_back(TreasuryPrices(0,0,-1));
		trade_side.push_back(BUY);
	}

	std::string side_string;
	std::string book("TRSY1");
	TreasuryPrices trade_max(100, 0, 0);

	//Create n_trades for each bond
	for (int i = 0; i < n_trades; i++) {

		for (int bond_index = 0; bond_index < bonds.size(); bond_index++) {

			if (bids[bond_index] == offers[bond_index]) {
				increment[bond_index].negate();

				bids[bond_index] = bids[bond_index] + increment[bond_index] * 2;
				offers[bond_index] = offers[bond_index] - increment[bond_index] * 2;
			}

			output << bonds[bond_index].GetProductId() << ",TradeID";

			if (trade_side[bond_index] == BUY) {
				output << "," << bids[bond_index];
				trade_side[bond_index] = SELL;
				side_string = "BUY";
			}
			else {
				output << "," << offers[bond_index];
				trade_side[bond_index] = BUY;
				side_string = "SELL";
			}

			output << "," << book;

			if (book == "TRSY1") {
				book = "TRSY2";
			}
			else if (book == "TRSY2") {
				book = "TRSY3";
			}
			else {
				book = "TRSY1";
			}

			output << "," << 1000000 * (1 + i % 6) << "," << side_string;
			output << std::endl;

			if (bids[bond_index].atMin() || offers[bond_index] == trade_max) {
				increment[bond_index].negate();
			}

			bids[bond_index]   = bids[bond_index]   + increment[bond_index];
			offers[bond_index] = offers[bond_index] - increment[bond_index];

		}

	}

	output.close();
}

#endif
