# Import the pyranha.math submodule.
from pyranha import math

# Various ways of constructing an int.
print(42)
print(int("42"))
print(int(42.123))

# Arbitrarily large ints are supported directly,
# no need to go through a string conversion.
print(12345678987654321234567898765432)

# Interoperability with float.
print(43. - 1)
# Truncated division in Python 2.x, floating-point division in Python 3.x.
print(85 / 2)

# Exponentiation via the '**' operator.
print(42**2)

# Calculate (42 choose 21) using the binomial() function from the math
# submodule.
print(math.binomial(42, 21))
