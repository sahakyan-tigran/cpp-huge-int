#include "HugeInt.h"

#include <algorithm>
#include <cctype>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>

namespace {

	int get_int(char c) { return c - '0'; }
	char get_char(int i) { return static_cast<char>(i + '0'); }

	bool is_number(const std::string& str) {
		if (str.empty()) {
			return false;        // "" used to be accepted and produced a
		}                        // zero-length number that printed nothing
		for (std::size_t i = 0; i < str.length(); ++i) {
			if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
				return false;
			}
		}
		return true;
	}
}

// Out-of-line definition so the constant can be odr-used (std::max takes its
// arguments by reference). C++17 makes constexpr members implicitly inline and
// this redundant, but it keeps the class building as C++11 and C++14 too.
constexpr int HugeInt::INITIAL_CAPACITY;

HugeInt::HugeInt(const std::string& value)
	: m_data(nullptr)
	, m_length(0)
	, m_capacity(0)
{
	if (!is_number(value)) {
		throw std::invalid_argument("HugeInt: expected a string of decimal digits");
	}

	int digits = static_cast<int>(value.length());
	m_capacity = std::max(digits, INITIAL_CAPACITY);
	m_data = new char[m_capacity]{};
	m_length = digits;
	for (int i = 0; i < digits; ++i) {
		m_data[m_capacity - digits + i] = value[static_cast<std::size_t>(i)];
	}
	strip_leading_zeros();
}

HugeInt::HugeInt(unsigned long long value)
	: m_data(new char[INITIAL_CAPACITY]{})
	, m_length(1)
	, m_capacity(INITIAL_CAPACITY)
{
	if (value == 0) {
		m_data[m_capacity - 1] = '0';
		return;
	}
	m_length = 0;
	for (int k = 0; value != 0; ++k, value /= 10) {
		m_data[m_capacity - 1 - k] = get_char(static_cast<int>(value % 10));
		++m_length;
	}
}

HugeInt::HugeInt(const HugeInt& arg)
	: m_data(new char[arg.m_capacity]{})
	, m_length(arg.m_length)
	, m_capacity(arg.m_capacity)
{
	for (int i = 0; i < m_capacity; ++i) {
		m_data[i] = arg.m_data[i];
	}
}

HugeInt& HugeInt::operator=(const HugeInt& arg) {
	if (this != &arg) {
		char* fresh = new char[arg.m_capacity]{};
		for (int i = 0; i < arg.m_capacity; ++i) {
			fresh[i] = arg.m_data[i];
		}
		deallocate();
		m_data = fresh;
		m_length = arg.m_length;
		m_capacity = arg.m_capacity;
	}
	return *this;
}

HugeInt::~HugeInt() {
	deallocate();
}

void HugeInt::deallocate() {
	delete[] m_data;
	m_data = nullptr;
}

int HugeInt::digit_from_right(int k) const {
	if (k < 0 || k >= m_length) {
		return 0;
	}
	return get_int(m_data[m_capacity - 1 - k]);
}

void HugeInt::set_digit_from_right(int k, int value) {
	m_data[m_capacity - 1 - k] = get_char(value);
}

void HugeInt::reserve(int capacity) {
	if (capacity <= m_capacity) {
		return;
	}
	char* fresh = new char[capacity]{};
	for (int k = 0; k < m_length; ++k) {
		fresh[capacity - 1 - k] = m_data[m_capacity - 1 - k];
	}
	delete[] m_data;
	m_data = fresh;
	m_capacity = capacity;
}

// Addition and multiplication both write a result one or two digits wider than
// strictly needed, so every result is normalised here. Zero keeps one digit.
void HugeInt::strip_leading_zeros() {
	while (m_length > 1 && m_data[m_capacity - m_length] == '0') {
		--m_length;
	}
	if (m_length == 0) {
		m_length = 1;
		m_data[m_capacity - 1] = '0';
	}
}

bool HugeInt::is_zero() const {
	return m_length == 1 && m_data[m_capacity - 1] == '0';
}

std::string HugeInt::to_string() const {
	return std::string(m_data + (m_capacity - m_length), static_cast<std::size_t>(m_length));
}

// Column addition. The old version computed its result length as
// max_length - i after the loop, which was always one digit too many, so the
// printed value carried a leading NUL byte.
HugeInt HugeInt::operator+(const HugeInt& arg) const {
	int result_length = std::max(m_length, arg.m_length) + 1;

	HugeInt result;
	result.reserve(std::max(result_length, INITIAL_CAPACITY));
	result.m_length = result_length;

	int carry = 0;
	for (int k = 0; k < result_length; ++k) {
		int sum = digit_from_right(k) + arg.digit_from_right(k) + carry;
		result.set_digit_from_right(k, sum % 10);
		carry = sum / 10;
	}
	result.strip_leading_zeros();
	return result;
}

// Schoolbook long multiplication.
//
// The old version multiplied into an `int` accumulator (via pow(), in double),
// then built a HugeInt from it -- so it overflowed at ten digits and defeated
// the entire point of the class.
HugeInt HugeInt::operator*(const HugeInt& arg) const {
	if (is_zero() || arg.is_zero()) {
		return HugeInt(0ull);
	}

	int result_length = m_length + arg.m_length;
	std::unique_ptr<int[]> columns(new int[result_length]());

	for (int i = 0; i < m_length; ++i) {
		int left = digit_from_right(i);
		if (left == 0) { continue; }
		for (int j = 0; j < arg.m_length; ++j) {
			columns[i + j] += left * arg.digit_from_right(j);
		}
	}

	int carry = 0;
	for (int k = 0; k < result_length; ++k) {
		int value = columns[k] + carry;
		columns[k] = value % 10;
		carry = value / 10;
	}
	// result_length = m + n is always enough for the product, so nothing is
	// left over here.

	HugeInt result;
	result.reserve(std::max(result_length, INITIAL_CAPACITY));
	result.m_length = result_length;
	for (int k = 0; k < result_length; ++k) {
		result.set_digit_from_right(k, columns[k]);
	}
	result.strip_leading_zeros();
	return result;
}

HugeInt& HugeInt::operator+=(const HugeInt& arg) {
	return *this = *this + arg;
}

HugeInt& HugeInt::operator*=(const HugeInt& arg) {
	return *this = *this * arg;
}

bool HugeInt::operator==(const HugeInt& arg) const {
	if (m_length != arg.m_length) {
		return false;
	}
	for (int k = 0; k < m_length; ++k) {
		if (digit_from_right(k) != arg.digit_from_right(k)) {
			return false;
		}
	}
	return true;
}

bool HugeInt::operator!=(const HugeInt& arg) const { return !(*this == arg); }

bool HugeInt::operator<(const HugeInt& arg) const {
	if (m_length != arg.m_length) {
		return m_length < arg.m_length;
	}
	for (int k = m_length - 1; k >= 0; --k) {       // most significant first
		int mine = digit_from_right(k);
		int theirs = arg.digit_from_right(k);
		if (mine != theirs) {
			return mine < theirs;
		}
	}
	return false;
}

bool HugeInt::operator>(const HugeInt& arg) const { return arg < *this; }
bool HugeInt::operator<=(const HugeInt& arg) const { return !(arg < *this); }
bool HugeInt::operator>=(const HugeInt& arg) const { return !(*this < arg); }

std::ostream& operator<<(std::ostream& out, const HugeInt& object) {
	out << object.to_string();     // no trailing endl: let the caller decide
	return out;
}

// Was declared but never defined, so any use of it failed to link.
std::istream& operator>>(std::istream& in, HugeInt& object) {
	std::string token;
	if (!(in >> token)) {
		return in;
	}
	try {
		object = HugeInt(token);
	}
	catch (const std::invalid_argument&) {
		in.setstate(std::ios::failbit);
	}
	return in;
}
