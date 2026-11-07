// Test suite for HugeInt.
//
//   g++ -std=c++17 -Wall -Wextra -I.. tests/test_hugeint.cpp ../HugeInt.cpp -o tests/test_hugeint
//   ./tests/test_hugeint
//
// Expected values were produced independently with Python's built-in bignums.

#include "../HugeInt.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& what) {
	++g_checks;
	if (!condition) {
		++g_failures;
		std::cout << "  FAIL: " << what << "\n";
	}
}

void check_eq(const std::string& actual, const std::string& expected, const std::string& what) {
	++g_checks;
	if (actual != expected) {
		++g_failures;
		std::cout << "  FAIL: " << what << "\n        got      " << actual
			<< "\n        expected " << expected << "\n";
	}
}

template <typename Fn>
bool throws(Fn fn) {
	try { fn(); }
	catch (const std::exception&) { return true; }
	return false;
}

void test_construction() {
	std::cout << "Construction\n";

	check_eq(HugeInt(0ull).to_string(), "0", "zero from integer");
	check_eq(HugeInt(7ull).to_string(), "7", "single digit from integer");
	check_eq(HugeInt(1234567890ull).to_string(), "1234567890", "ten digits from integer");
	check_eq(HugeInt(18446744073709551615ull).to_string(), "18446744073709551615",
		"unsigned long long max");

	check_eq(HugeInt(std::string("0")).to_string(), "0", "zero from string");
	check_eq(HugeInt(std::string("42")).to_string(), "42", "small from string");

	// A number far wider than any built-in type.
	const std::string huge = "123456789012345678901234567890123456789012345678901234567890";
	check_eq(HugeInt(huge).to_string(), huge, "60 digits round-trip");

	// Leading zeros are normalised away.
	check_eq(HugeInt(std::string("000123")).to_string(), "123", "leading zeros stripped");
	check_eq(HugeInt(std::string("0000")).to_string(), "0", "all zeros collapse to 0");

	// The empty string used to be accepted and produced a number that printed
	// nothing at all.
	check(throws([] { HugeInt(std::string("")); }), "empty string rejected");
	check(throws([] { HugeInt(std::string("12a3")); }), "non-digit rejected");
	check(throws([] { HugeInt(std::string("-5")); }), "minus sign rejected");
	check(throws([] { HugeInt(std::string(" 12")); }), "leading space rejected");

	check(HugeInt(0ull).is_zero(), "is_zero on zero");
	check(!HugeInt(1ull).is_zero(), "is_zero on one");
	check(HugeInt(std::string("12345")).length() == 5, "length");
}

void test_copy() {
	std::cout << "Copy and assignment\n";

	HugeInt a(std::string("987654321098765432109876543210"));
	HugeInt b = a;
	check_eq(b.to_string(), a.to_string(), "copy constructor");

	HugeInt c(1ull);
	c = a;
	check_eq(c.to_string(), a.to_string(), "assignment");

	// Mutating the copy must not touch the source.
	c += HugeInt(1ull);
	check_eq(a.to_string(), "987654321098765432109876543210", "source unchanged by copy");

	a = a;                                   // self-assignment must be safe
	check_eq(a.to_string(), "987654321098765432109876543210", "self-assignment");

	// Assigning a wider value over a narrower one must grow the buffer.
	HugeInt small(5ull);
	small = HugeInt(std::string(
		"11111111112222222222333333333344444444445555555555666666666677777777778888888888"));
	check_eq(small.to_string(),
		"11111111112222222222333333333344444444445555555555666666666677777777778888888888",
		"assignment widens the buffer");
}

void test_addition() {
	std::cout << "Addition\n";

	// The old operator+ reported a result one digit too long, so even 1 + 1
	// printed a stray NUL byte before the digits.
	check_eq((HugeInt(1ull) + HugeInt(1ull)).to_string(), "2", "1 + 1");
	check_eq((HugeInt(0ull) + HugeInt(0ull)).to_string(), "0", "0 + 0");
	check_eq((HugeInt(0ull) + HugeInt(123ull)).to_string(), "123", "0 + n");
	check_eq((HugeInt(123ull) + HugeInt(0ull)).to_string(), "123", "n + 0");

	check_eq((HugeInt(5ull) + HugeInt(5ull)).to_string(), "10", "carry into a new digit");
	check_eq((HugeInt(99ull) + HugeInt(1ull)).to_string(), "100", "cascading carry");
	check_eq((HugeInt(999999999ull) + HugeInt(1ull)).to_string(), "1000000000", "long carry");

	// Operands of different widths, both ways round.
	check_eq((HugeInt(std::string("1000000000000000000000")) + HugeInt(1ull)).to_string(),
		"1000000000000000000001", "wide + narrow");
	check_eq((HugeInt(1ull) + HugeInt(std::string("1000000000000000000000"))).to_string(),
		"1000000000000000000001", "narrow + wide");

	check_eq((HugeInt(std::string("12345678901234567890"))
		+ HugeInt(std::string("98765432109876543210"))).to_string(),
		"111111111011111111100", "twenty-digit addition");

	// All nines: every column carries.
	check_eq((HugeInt(std::string("99999999999999999999")) + HugeInt(1ull)).to_string(),
		"100000000000000000000", "all nines plus one");

	HugeInt acc(0ull);
	for (int i = 0; i < 1000; ++i) {
		acc += HugeInt(std::string("1000000000000000000000000000000"));
	}
	check_eq(acc.to_string(), "1000000000000000000000000000000000", "1000 accumulations");
}

void test_multiplication() {
	std::cout << "Multiplication\n";

	check_eq((HugeInt(0ull) * HugeInt(12345ull)).to_string(), "0", "0 * n");
	check_eq((HugeInt(12345ull) * HugeInt(0ull)).to_string(), "0", "n * 0");
	check_eq((HugeInt(1ull) * HugeInt(12345ull)).to_string(), "12345", "1 * n");
	check_eq((HugeInt(6ull) * HugeInt(7ull)).to_string(), "42", "single digits");
	check_eq((HugeInt(99ull) * HugeInt(99ull)).to_string(), "9801", "two digits");

	// This is the case the old implementation could not do at all: it summed
	// into an int via pow(), so anything past ten digits overflowed.
	check_eq((HugeInt(std::string("12345678901234567890"))
		* HugeInt(std::string("98765432109876543210"))).to_string(),
		"1219326311370217952237463801111263526900", "twenty-digit multiplication");

	// 2^100, by repeated doubling.
	HugeInt power(1ull);
	for (int i = 0; i < 100; ++i) {
		power *= HugeInt(2ull);
	}
	check_eq(power.to_string(), "1267650600228229401496703205376", "2^100");

	// 50!
	HugeInt factorial(1ull);
	for (unsigned long long i = 2; i <= 50; ++i) {
		factorial *= HugeInt(i);
	}
	check_eq(factorial.to_string(),
		"30414093201713378043612608166064768844377641568960512000000000000", "50!");

	// Commutativity on a wide pair.
	HugeInt x(std::string("8675309867530986753098675309"));
	HugeInt y(std::string("31415926535897932384626433832795"));
	check_eq((x * y).to_string(), (y * x).to_string(), "multiplication commutes");

	// Distributivity: a * (b + c) == a*b + a*c.
	HugeInt a(std::string("123456789123456789"));
	HugeInt b(std::string("987654321987654321"));
	HugeInt c(std::string("555555555555555555"));
	check_eq((a * (b + c)).to_string(), (a * b + a * c).to_string(), "multiplication distributes");
}

void test_comparison() {
	std::cout << "Comparison\n";

	HugeInt small(std::string("999"));
	HugeInt big(std::string("1000"));
	HugeInt same(std::string("999"));

	check(small == same, "equal values compare equal");
	check(!(small == big), "different values are not equal");
	check(small != big, "operator!=");
	check(small < big, "fewer digits is smaller");
	check(big > small, "more digits is larger");
	check(small <= same, "operator<= on equals");
	check(small >= same, "operator>= on equals");
	check(!(big < small), "operator< is not symmetric");

	// Same width, differing in the middle.
	check(HugeInt(std::string("123456789")) < HugeInt(std::string("123466789")),
		"same width, differing digit");
	check(!(HugeInt(std::string("123466789")) < HugeInt(std::string("123456789"))),
		"same width, reverse");

	// Leading zeros must not affect ordering.
	check(HugeInt(std::string("0099")) < HugeInt(std::string("100")),
		"leading zeros ignored in comparison");
}

void test_streams() {
	std::cout << "Streams\n";

	std::ostringstream out;
	out << HugeInt(std::string("123456789"));
	check_eq(out.str(), "123456789", "operator<< writes the digits");

	// operator<< used to append std::endl, so it could not be composed.
	std::ostringstream composed;
	composed << "[" << HugeInt(42ull) << "]";
	check_eq(composed.str(), "[42]", "operator<< adds no newline");

	// operator>> was declared but never defined, so using it failed to link.
	std::istringstream in("987654321098765432109876543210");
	HugeInt read(0ull);
	in >> read;
	check(!in.fail(), "operator>> succeeds on valid input");
	check_eq(read.to_string(), "987654321098765432109876543210", "operator>> reads the value");

	std::istringstream bad("12x34");
	HugeInt unused(0ull);
	bad >> unused;
	check(bad.fail(), "operator>> sets failbit on invalid input");
}

}  // namespace

int main() {
	test_construction();
	test_copy();
	test_addition();
	test_multiplication();
	test_comparison();
	test_streams();

	std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
	if (g_failures != 0) {
		std::cout << g_failures << " FAILED\n";
		return 1;
	}
	std::cout << "All tests passed.\n";
	return 0;
}
