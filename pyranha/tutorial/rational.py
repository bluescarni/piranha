# Import the pyranha.math submodule.
from pyranha import math
# Import the standard Fraction class.
from fractions import Fraction as F

# Various ways of constructing a rational.
print(F(42))
print(F("42"))
print(F(1.5))
print(F(42, 12))
print(F("42/12"))

# Arithmetics and interoperability with float and int.
print(F(42, 13) * 2)
print(1 + F(42, 13))
print(43. - F(1, 2))
print(F(85) / 13)
print(F(84, 2) == 42)
print(42 >= F(84, 3))

# Exponentiation is provided by the standard '**' operator.
print(F(42, 13)**2)
print(F(42, 13)**-3)

# Conversion to int yields the truncated value.
print(int(F(10, 3)))

# Calculate (42/13 choose 21) via the math::binomial() function.
print(math.binomial(F(42, 13), 21))
