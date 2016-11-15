.. Piranha documentation master file, created by
   sphinx-quickstart on Sun Nov  6 03:43:00 2011.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the Piranha documentation!
*************************************

Piranha is a computer-algebra library for the symbolic manipulation of sparse
multivariate polynomials and other closely-related symbolic objects
(such as Poisson series) commonly encountered in celestial mechanics.

Piranha is written in modern C++, with emphasis on portability, correctness
and performance. Piranha also includes a set of optional bindings for the
Python programming language, called **Pyranha**, that allow to use the
library in an interactive and script-oriented way.

.. warning:: This documentation is a (currently incomplete) work in progress.

Quick start
===========

A quick taste of Piranha from C++:

.. code-block:: c++

   #include <cstdint>
   #include <iostream>

   #include <piranha/piranha.hpp>

   using namespace piranha;

   int main()
   {
      // Initialise the library.
      init();

      // Define a polynomial type over Q, with 16-bit signed exponents.
      using pt = polynomial<rational,monomial<std::int16_t>>;

      // Create polynomials representing the symbolic
      // variables 'x', 'y' and 'z'.
      pt x{"x"}, y{"y"}, z{"z"};

      // Compute and print (x+y+2*z-1)**10.
      auto res = math::pow(x + y + 2*z - 1, 10);
      std::cout << res << '\n';

      // Print the degree of 'res' in the variables 'x' and 'y'.
      std::cout << math::degree(res,{"x","y"}) << '\n';

      // Evaluate 'res' using double-precision floats in
      // x = 1.5 , y = -3.5 and z = 1.2.
      std::cout << math::evaluate<double>(res,{{"x",1.5},{"y",-3.5},{"z",1.2}}) << '\n';

      // Compute an antiderivative of 'res' with respect to 'x'.
      auto int_res_x = math::integrate(res,"x");
      std::cout << int_res_x << '\n';

      // Verify consistency with partial differentiation.
      std::cout << (res == math::partial(int_res_x,"x")) << '\n';
   }

The equivalent Python code:

.. code-block:: python

   >>> from pyranha.types import polynomial, rational, monomial, int16
   >>> import pyranha.math as math
   >>> pt = polynomial[rational,monomial[int16]]()
   >>> x,y,z = pt('x'), pt('y'), pt('z')
   >>> res = (x + y + 2*z - 1)**10
   >>> res
   10080*x**3*y**5*z**2-30240*x*y**2*z**2+720*x**7...
   >>> math.degree(res,['x','y'])
   10
   >>> math.evaluate(res,{'x':1.5,'y':-3.5,'z':1.2})
   0.00604647211...
   >>> int_res_x = math.integrate(res,'x')
   >>> int_res_x
   -10080*x**3*y**5*z**2+5040*x*y**2*z**2-720*x**7*y**2*z+...
   >>> res == math.partial(int_res_x,'x')
   True

Scope and limitations
=====================

Piranha was originally written to support research via perturbative methods in celestial mechanics (although
eventually it has proven useful also in other contexts, see below). Piranha might be the right tool for you if:

* you are a researcher in celestial mechanics;
* you need to perform simple arithmetics and differential operations on very large sparse multivariate polynomials
  and/or Fourier/Poisson series;
* you need to work with polynomials and other related algebraic structures defined in terms of arbitrarily generic
  types.

On the other hand, you might want to use other systems in these situations:

* you need to work with dense univariate polynomials: Piranha is optimised for sparse polynomials and it will have
  very poor performance when operating on dense polynomials. You might want to check out
  `Flint <http://flintlib.org/>`__ instead;
* you need to work with general symbolic expressions: Piranha is a system specialised to operate on very specific
  algebraic objects, if you need a general-purpose system you should try `SymPy <http://www.sympy.org/>`__,
  `SAGE <http://www.sagemath.org/>`__, or one of the many open-source CASs (e.g., see
  `here <https://en.wikipedia.org/wiki/List_of_computer_algebra_systems>`__);
* you need polynomial manipulation capabilities currently not available in Piranha (e.g., Gröbner bases, GCD,
  factorization, etc.). Piranha was designed to be as fast as possible for a small set of fundamental arithmetic
  operations commonly needed in celestial mechanics. If you need more advanced symbolic polynomial capabilities,
  you might want to check out projects such as `Singular <https://www.singular.uni-kl.de/>`__ and
  `PARI <http://pari.math.u-bordeaux.fr/>`__.

Who is using Piranha?
=====================

Contents
========

.. toctree::
   :maxdepth: 2

   getting_started.rst
   basic_concepts.rst
   reference.rst
   appendix.rst

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
