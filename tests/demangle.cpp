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

#include "../src/detail/demangle.hpp"

#define BOOST_TEST_MODULE demangle_test
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_set>
#include <vector>

#include "../src/init.hpp"

using namespace piranha;
using detail::demangle;

// Check the macro definition is not leaking.
static bool phbd_defined =
#if defined(PIRANHA_HAVE_BOOST_DEMANGLE)
    true;
#else
    false;
#endif

struct base_foo {
    virtual void f() const
    {
    }
    virtual ~base_foo()
    {
    }
};

struct foo : base_foo {
};

namespace myns
{

template <typename T>
struct bar {
};
}

BOOST_AUTO_TEST_CASE(demangle_test)
{
    init();
    BOOST_CHECK(!phbd_defined);
    // Likely not a valid mangled name.
    std::cout << demangle("helloworld!") << '\n';
    std::cout << demangle("") << '\n';
    std::cout << demangle(std::string("")) << '\n';
    // A few valid types.
    std::cout << demangle<int>() << '\n';
    std::cout << demangle<std::vector<int>>() << '\n';
    std::cout << demangle(std::type_index(typeid(std::vector<std::string>{}))) << '\n';
    std::cout << demangle(typeid(std::unordered_set<std::string>{})) << '\n';
    std::cout << demangle(typeid(std::unordered_set<std::string>{}).name()) << '\n';
    std::cout << demangle(std::string(typeid(std::unordered_set<std::string>{}).name())) << '\n';
    std::cout << demangle<base_foo>() << '\n';
    std::cout << demangle<foo>() << '\n';
    std::cout << demangle<myns::bar<int>>() << '\n';
    // Check with dynamic polymorphism.
    std::unique_ptr<base_foo> foo_ptr(new foo{});
    BOOST_CHECK_EQUAL(demangle(typeid(*foo_ptr.get())), demangle<foo>());
}
