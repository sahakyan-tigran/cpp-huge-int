#pragma once

#include <iosfwd>
#include <string>

// Arbitrary-precision non-negative integer.
//
// Digits are stored as characters in a manually managed buffer, right-aligned:
// the number occupies the last m_length bytes of m_data, most significant
// first, and the unused leading bytes are zero. The buffer grows as needed, so
// the only limit is memory.
class HugeInt {
	friend std::ostream& operator<<(std::ostream&, const HugeInt&);
	friend std::istream& operator>>(std::istream&, HugeInt&);

public:
	HugeInt(const std::string&);
	explicit HugeInt(unsigned long long = 0);
	HugeInt(const HugeInt&);
	HugeInt& operator=(const HugeInt&);
	~HugeInt();

	HugeInt operator+(const HugeInt&) const;
	HugeInt operator*(const HugeInt&) const;
	HugeInt& operator+=(const HugeInt&);
	HugeInt& operator*=(const HugeInt&);

	bool operator==(const HugeInt&) const;
	bool operator!=(const HugeInt&) const;
	bool operator<(const HugeInt&) const;
	bool operator>(const HugeInt&) const;
	bool operator<=(const HugeInt&) const;
	bool operator>=(const HugeInt&) const;

	// Number of decimal digits. Zero has length 1.
	int length() const { return m_length; }
	bool is_zero() const;
	std::string to_string() const;

private:
	// k counts from the right: 0 is the ones place. Out-of-range reads give 0,
	// which is what makes the addition and multiplication loops uniform.
	int digit_from_right(int k) const;
	void set_digit_from_right(int k, int value);

	void reserve(int capacity);        // ensure room for `capacity` digits
	void strip_leading_zeros();
	void deallocate();

private:
	static constexpr int INITIAL_CAPACITY = 40;

	char* m_data;
	int m_length;
	int m_capacity;
};

std::ostream& operator<<(std::ostream&, const HugeInt&);
std::istream& operator>>(std::istream&, HugeInt&);
