# Huge Integer

An arbitrary-precision unsigned integer in C++, written from scratch with
manual memory management — no `std::vector`, no `__int128`, no GMP.

The point of the exercise is the bookkeeping: a hand-managed `char*` buffer,
the rule of three, operator overloading, and schoolbook arithmetic that stays
correct no matter how many digits you throw at it.

![tests](https://github.com/sahakyan-tigran/cpp-huge-int/actions/workflows/ci.yml/badge.svg)

## Representation

Digits are stored as characters, right-aligned in the buffer:

```
capacity = 40, length = 5, value = 12345

index:  0 ....................... 34 35 36 37 38 39
data:  \0 \0 \0 ... \0 \0 \0 \0 \0  1  2  3  4  5
                                    ^
                                    capacity - length
```

Right-aligning means the ones place is always at `capacity - 1`, so the
addition and multiplication loops index from the right and never have to
special-case operands of different widths. `digit_from_right(k)` returns 0 for
any `k` past the end, which is what keeps those loops branch-free.

The buffer grows on demand, so the only limit is available memory.

## Building and testing

```sh
make test
```

or directly:

```sh
g++ -std=c++17 -Wall -Wextra -I. tests/test_hugeint.cpp HugeInt.cpp -o test_hugeint && ./test_hugeint
```

The suite checks construction, copying and self-assignment, both arithmetic
operators, ordering, and the stream operators, including 2^100 by repeated
doubling and 50! by repeated multiplication. The expected values were produced
independently with Python's built-in bignums.

## Usage

```cpp
#include "HugeInt.h"
#include <iostream>

int main() {
    HugeInt a("12345678901234567890");
    HugeInt b("98765432109876543210");

    std::cout << a + b << "\n";    // 111111111011111111100
    std::cout << a * b << "\n";    // 1219326311370217952237463801111263526900

    HugeInt factorial(1);
    for (unsigned long long i = 2; i <= 50; ++i) {
        factorial *= HugeInt(i);
    }
    std::cout << factorial << "\n";
    // 30414093201713378043612608166064768844377641568960512000000000000
}
```

## API

| Member | Notes |
| --- | --- |
| `HugeInt(const std::string&)` | Throws `std::invalid_argument` on anything that is not a non-empty run of digits. Leading zeros are normalised away. |
| `explicit HugeInt(unsigned long long = 0)` | Defaults to zero. |
| `operator+`, `operator*`, `operator+=`, `operator*=` | Addition is column addition; multiplication is schoolbook long multiplication. |
| `operator==` `!=` `<` `>` `<=` `>=` | Compares by length, then digit by digit. |
| `length()`, `is_zero()`, `to_string()` | |
| `operator<<`, `operator>>` | `>>` reads one whitespace-delimited token and sets `failbit` if it is not a valid number. |

## Limitations

- Unsigned only. There is no subtraction, division or modulo, so there is
  nothing for a sign to do yet. Adding subtraction is the natural next step
  and is what would force a sign bit.
- Multiplication is O(n·m). Karatsuba would pay off somewhere in the low
  hundreds of digits; at the sizes this is used for, the simple algorithm wins
  on constant factors.
- Copyable but not movable, which costs a buffer copy on every return. The
  returns are the obvious place to add move semantics.

## License

MIT
