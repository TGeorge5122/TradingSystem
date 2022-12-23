/**
 * treasuryprices.hpp
 * Implements treasurying pricing notation and conversion to decimal
 *
 */
#ifndef TREASURY_PRICES_HPP
#define TREASURY_PRICES_HPP

#include <cstdio>

//Class to handle treasury price notation
class TreasuryPrices {
private:

	int integer;
	int xy;
	int z;

public:
	
	TreasuryPrices();

	TreasuryPrices(const std::string&);

	TreasuryPrices(int, int, int);

	TreasuryPrices operator* (const int);

	TreasuryPrices operator+ (const TreasuryPrices&);

	TreasuryPrices operator- (const TreasuryPrices&);

	bool operator== (const TreasuryPrices&);

	//Convert to proper treasury price notation
	TreasuryPrices standardize(TreasuryPrices t);

	//At bottom of oscillation range
	bool atMin();

	//At top of oscillation range
	bool atMax();

	//Above oscillation range
	bool gtMax();

	//Convert to decimal notation
	double toDouble();

	//Used for midpoint increment - differs from * -1 since we don't call standardize
	void negate();

	int getInteger() const;
	
	int getXY() const;
	
	int getZ() const;

};

TreasuryPrices::TreasuryPrices() {
	integer = 0;
	xy = 0;
	z = 0;
}

TreasuryPrices::TreasuryPrices(const std::string& s) {

	std::string::size_type delim_position = s.find("-");

	//Pre-c++11
	std::sscanf(s.substr(0, delim_position).c_str(),"%d", &integer);
	std::sscanf(s.substr(delim_position + 1, 2).c_str(),"%d", &xy);
	std::sscanf(s.substr(delim_position + 3, 1).c_str(),"%d", &z);
}

TreasuryPrices::TreasuryPrices(int i_, int xy_, int z_) {
	integer = i_;
	xy = xy_;
	z = z_;
}

TreasuryPrices TreasuryPrices::standardize(TreasuryPrices t) {

	int remainder = t.getInteger() * 256 + t.getXY() * 8 + t.getZ();
	int new_integer = remainder / 256;
	remainder -= new_integer * 256;
	int new_xy = remainder / 8;

	return TreasuryPrices(new_integer, new_xy, remainder - new_xy * 8);

}

TreasuryPrices TreasuryPrices::operator* (const int scalar) {
	return standardize(TreasuryPrices(integer * scalar, xy * scalar, z * scalar));
}

TreasuryPrices TreasuryPrices::operator+ (const TreasuryPrices& other) {
	return standardize(TreasuryPrices(integer + other.getInteger(), xy + other.getXY(), z + other.getZ()));
}

TreasuryPrices TreasuryPrices::operator- (const TreasuryPrices& other) {
	return standardize(TreasuryPrices(integer - other.getInteger(), xy - other.getXY(), z - other.getZ()));
}

bool TreasuryPrices::operator== (const TreasuryPrices& other) {
	return integer == other.getInteger() && xy == other.getXY() && z == other.getZ();
}

//Hardcoding 99 as minimum 
bool TreasuryPrices::atMin() {
	return integer == 99 && xy == 0 && z == 0;
}

//Hardcoding 101 as maximum
bool TreasuryPrices::atMax() {
	return integer == 101;
}

//Hardcoding 101 as maximum
bool TreasuryPrices::gtMax() {
	return !((integer < 101) || (integer == 101 && xy == 0 && z == 0));
}

double TreasuryPrices::toDouble() {
	return integer + xy / 32.0 + z / 256.0;
}

void TreasuryPrices::negate() {
	integer *= -1;
	xy *= -1;
	z *= -1;
}

int TreasuryPrices::getInteger() const {
	return integer;
}

int TreasuryPrices::getXY() const {
	return xy;
}

int TreasuryPrices::getZ() const {
	return z;
}

//Output treasury price as integer-xyz
std::ostream& operator<< (std::ostream& os, const TreasuryPrices& t) {

	os << t.getInteger() << "-";

	if (t.getXY() < 10) {
		os << "0";
	}
		
	os << t.getXY();

	if (t.getZ() == 4) {
		os << "+";
	}
	else {
		os << t.getZ();
	}

	return os;
}

#endif
