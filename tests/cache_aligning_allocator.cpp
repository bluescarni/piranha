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

#include <piranha/cache_aligning_allocator.hpp>

#define BOOST_TEST_MODULE cache_aligning_allocator_test
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <type_traits>

#include <piranha/init.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/runtime_info.hpp>
#include <piranha/settings.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

BOOST_AUTO_TEST_CASE(cache_aligning_allocator_constructor_test)
{
    init();
    cache_aligning_allocator<char> caa1;
    BOOST_CHECK_EQUAL(caa1.alignment(), runtime_info::get_cache_line_size());
    cache_aligning_allocator<integer> caa2;
    BOOST_CHECK_EQUAL(caa2.alignment(), runtime_info::get_cache_line_size());
    cache_aligning_allocator<std::string> caa3;
    BOOST_CHECK_EQUAL(caa3.alignment(), runtime_info::get_cache_line_size());
    // Constructor from different instance.
    cache_aligning_allocator<int> caa4(caa1);
    BOOST_CHECK_EQUAL(caa4.alignment(), caa1.alignment());
    // Constructor from different instance.
    cache_aligning_allocator<int> caa5(std::move(caa1));
    BOOST_CHECK_EQUAL(caa4.alignment(), caa5.alignment());
    settings::set_cache_line_size(settings::get_cache_line_size() * 2u);
    cache_aligning_allocator<char> caa6;
    BOOST_CHECK(caa6.alignment() == settings::get_cache_line_size());
    cache_aligning_allocator<int> caa7(std::move(caa6));
    BOOST_CHECK(caa7.alignment() == settings::get_cache_line_size());
    settings::set_cache_line_size(3);
    cache_aligning_allocator<int> caa8;
    BOOST_CHECK(caa8.alignment() == 0u);
    BOOST_CHECK(cache_aligning_allocator<int>::propagate_on_container_move_assignment::value);
    BOOST_CHECK(is_container_element<cache_aligning_allocator<int>>::value);
}

PIRANHA_DECLARE_HAS_TYPEDEF(pointer);
PIRANHA_DECLARE_HAS_TYPEDEF(const_pointer);
PIRANHA_DECLARE_HAS_TYPEDEF(reference);
PIRANHA_DECLARE_HAS_TYPEDEF(const_reference);

BOOST_AUTO_TEST_CASE(cache_aligning_allocator_construct_destroy_test)
{
    cache_aligning_allocator<char> caa1;
    char c1;
    caa1.construct(&c1, 'f');
    BOOST_CHECK(c1 == 'f');
    caa1.destroy(&c1);
    std::aligned_storage<sizeof(std::string), alignof(std::string)>::type st1;
    cache_aligning_allocator<std::string> caa2;
    caa2.construct(static_cast<std::string *>(static_cast<void *>(&st1)), std::string("foo"));
    BOOST_CHECK(*static_cast<std::string *>(static_cast<void *>(&st1)) == "foo");
    caa2.destroy(static_cast<std::string *>(static_cast<void *>(&st1)));
    caa2.construct(static_cast<std::string *>(static_cast<void *>(&st1)), "bar");
    BOOST_CHECK(*static_cast<std::string *>(static_cast<void *>(&st1)) == "bar");
    caa2.destroy(static_cast<std::string *>(static_cast<void *>(&st1)));
    caa2.construct(static_cast<std::string *>(static_cast<void *>(&st1)));
    BOOST_CHECK(*static_cast<std::string *>(static_cast<void *>(&st1)) == "");
    caa2.destroy(static_cast<std::string *>(static_cast<void *>(&st1)));
    BOOST_CHECK(cache_aligning_allocator<std::string>::rebind<char>::other{} == caa1);
    BOOST_CHECK(has_typedef_pointer<cache_aligning_allocator<std::string>>::value);
    BOOST_CHECK(has_typedef_const_pointer<cache_aligning_allocator<std::string>>::value);
    BOOST_CHECK(has_typedef_reference<cache_aligning_allocator<std::string>>::value);
    BOOST_CHECK(has_typedef_const_reference<cache_aligning_allocator<std::string>>::value);
}
