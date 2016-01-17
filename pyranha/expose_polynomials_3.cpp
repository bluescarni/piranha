/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "python_includes.hpp"

#include "../src/polynomial.hpp"
#include "expose_polynomials.hpp"
#include "expose_utils.hpp"
#include "polynomial_descriptor.hpp"

namespace pyranha
{

void expose_polynomials_3()
{
	series_exposer<piranha::polynomial,polynomial_descriptor,9u,12u,poly_custom_hook<polynomial_descriptor>> poly_exposer;
}

}
