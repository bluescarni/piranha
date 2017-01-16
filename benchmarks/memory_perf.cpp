/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#include <piranha/memory.hpp>

#define BOOST_TEST_MODULE memory_test
#include <boost/test/included/unit_test.hpp>

#include <array>
#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include <piranha/detail/demangle.hpp>
#include <piranha/init.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/settings.hpp>

#include "simple_timer.hpp"

using namespace piranha;

// Size of the parallel arrays to allocate.
static const std::size_t alloc_size = 20000000ull;

struct array_wrap {
    array_wrap()
    {
        m_array[0] = 0;
    }
    std::array<int, 5> m_array;
};

class custom_string : public std::string
{
public:
    custom_string() : std::string("hello")
    {
    }
    custom_string(const custom_string &) = default;
    custom_string(custom_string &&other) noexcept : std::string(std::move(other))
    {
    }
    template <typename... Args>
    custom_string(Args &&... params) : std::string(std::forward<Args>(params)...)
    {
    }
    custom_string &operator=(const custom_string &) = default;
    custom_string &operator=(custom_string &&other) noexcept
    {
        std::string::operator=(std::move(other));
        return *this;
    }
    ~custom_string() noexcept
    {
    }
};

BOOST_AUTO_TEST_CASE(memory_parallel_array_test)
{
    init();
    settings::set_thread_binding(true);
    if (boost::unit_test::framework::master_test_suite().argc > 1) {
        settings::set_n_threads(
            boost::lexical_cast<unsigned>(boost::unit_test::framework::master_test_suite().argv[1u]));
    }
    try {
        std::cout << "Testing int\n"
                     "===========\n";
        for (unsigned i = 0u; i < settings::get_n_threads(); ++i) {
            std::cout << "n = " << i + 1u << '\n';
            simple_timer t;
            auto ptr1 = make_parallel_array<int>(alloc_size, i + 1u);
        }
        std::cout << "Testing string\n"
                     "==============\n";
        for (unsigned i = 0u; i < settings::get_n_threads(); ++i) {
            std::cout << "n = " << i + 1u << '\n';
            simple_timer t;
            auto ptr1 = make_parallel_array<custom_string>(alloc_size, i + 1u);
        }
        std::cout << "Testing integer\n"
                     "===============\n";
        for (unsigned i = 0u; i < settings::get_n_threads(); ++i) {
            std::cout << "n = " << i + 1u << '\n';
            simple_timer t;
            auto ptr1 = make_parallel_array<integer>(alloc_size, i + 1u);
        }
        std::cout << "Testing mp_integer\n"
                     "==================\n";
        for (unsigned i = 0u; i < settings::get_n_threads(); ++i) {
            std::cout << "n = " << i + 1u << '\n';
            simple_timer t;
            auto ptr1 = make_parallel_array<mp_integer<>>(alloc_size, i + 1u);
        }
        std::cout << "Testing array wrap\n"
                     "==================\n";
        for (unsigned i = 0u; i < settings::get_n_threads(); ++i) {
            std::cout << "n = " << i + 1u << '\n';
            simple_timer t;
            auto ptr1 = make_parallel_array<array_wrap>(alloc_size, i + 1u);
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception caught, type is '" << detail::demangle(typeid(e)) << "', message is: " << e.what()
                  << '\n';
        std::cout << "Exception caught, type is '" << detail::demangle(typeid(e)) << "', message is: " << e.what()
                  << '\n';
        throw;
    }
}
