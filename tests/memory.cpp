/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "../src/memory.hpp"

#define BOOST_TEST_MODULE memory_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <boost/timer/timer.hpp>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <new>
#include <vector>

#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/settings.hpp"

using namespace piranha;

BOOST_AUTO_TEST_CASE(memory_aligned_malloc_test)
{
	environment env;
	// Test the common codepath.
	auto ptr = aligned_palloc(0,0);
	BOOST_CHECK(ptr == nullptr);
	BOOST_CHECK_NO_THROW(aligned_pfree(0,ptr));
	ptr = aligned_palloc(123,0);
	BOOST_CHECK(ptr == nullptr);
	BOOST_CHECK_NO_THROW(aligned_pfree(123,ptr));
	ptr = aligned_palloc(0,1);
	BOOST_CHECK(ptr != nullptr);
	BOOST_CHECK_NO_THROW(aligned_pfree(0,ptr));
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
	// posix_memalign requires power of two and multiple of sizeof(void *) for alignment value.
	BOOST_CHECK_THROW(aligned_palloc(3,1),std::bad_alloc);
	BOOST_CHECK_THROW(aligned_palloc(7,1),std::bad_alloc);
	// For these tests, require that the alignment is valid for int and that it is a power of 2.
	if (sizeof(void *) % alignof(int) == 0 && !(sizeof(void *) & (sizeof(void *) - 1u))) {
		ptr = aligned_palloc(sizeof(void *),sizeof(int) * 10000u);
		std::vector<int> v;
		std::generate_n(std::back_inserter(v),10000u,std::rand);
		std::copy(v.begin(),v.end(),static_cast<int *>(ptr));
		BOOST_CHECK(std::equal(v.begin(),v.end(),static_cast<int *>(ptr)));
		BOOST_CHECK_NO_THROW(aligned_pfree(sizeof(void *),ptr));
	}
#elif defined(_WIN32)
	// _aligned_malloc requires power of two.
	BOOST_CHECK_THROW(aligned_palloc(3,1),std::bad_alloc);
	BOOST_CHECK_THROW(aligned_palloc(7,1),std::bad_alloc);
	// Check that the alignment is valid for int.
	if (16 % alignof(int) == 0) {
		ptr = aligned_palloc(16,sizeof(int) * 10000u);
		std::vector<int> v;
		std::generate_n(std::back_inserter(v),10000u,std::rand);
		std::copy(v.begin(),v.end(),static_cast<int *>(ptr));
		BOOST_CHECK(std::equal(v.begin(),v.end(),static_cast<int *>(ptr)));
		BOOST_CHECK_NO_THROW(aligned_pfree(16,ptr));
	}
#endif
}

// NOTE: here we are assuming we can do some basic arithmetics on the alignment and size values.
BOOST_AUTO_TEST_CASE(memory_alignment_check_test)
{
	BOOST_CHECK(piranha::alignment_check<int>(0));
	BOOST_CHECK(piranha::alignment_check<long long>(0));
	BOOST_CHECK(piranha::alignment_check<std::string>(0));
#if defined(PIRANHA_HAVE_POSIX_MEMALIGN)
	// posix_memalign has additional requirements.
	if (sizeof(void *) >= alignof(int) && !(sizeof(void *) & ((sizeof(void *) - 1u)))) {
		// sizeof(void *) is a power of two greater than the alignment of int.
		BOOST_CHECK(piranha::alignment_check<int>(sizeof(void *) * 2u));
		BOOST_CHECK(piranha::alignment_check<int>(sizeof(void *) * 4u));
		BOOST_CHECK(piranha::alignment_check<int>(sizeof(void *) * 8u));
	}
	if (sizeof(void *) >= alignof(long) && !(sizeof(void *) & ((sizeof(void *) - 1u)))) {
		BOOST_CHECK(piranha::alignment_check<long>(sizeof(void *) * 2u));
		BOOST_CHECK(piranha::alignment_check<long>(sizeof(void *) * 4u));
		BOOST_CHECK(piranha::alignment_check<long>(sizeof(void *) * 8u));
	}
#else
	BOOST_CHECK(piranha::alignment_check<int>(alignof(int)));
	BOOST_CHECK(piranha::alignment_check<std::string>(alignof(std::string)));
	BOOST_CHECK(piranha::alignment_check<int>(alignof(int) * 2u));
	BOOST_CHECK(piranha::alignment_check<std::string>(alignof(std::string) * 4u));
	BOOST_CHECK(piranha::alignment_check<std::string>(alignof(std::string) * 8u));
	if (alignof(int) > 1u) {
		BOOST_CHECK(!piranha::alignment_check<int>(alignof(int) / 2u));
	}
	if (alignof(long) > 1u) {
		BOOST_CHECK(!piranha::alignment_check<long>(alignof(long) / 2u));
	}
	if (alignof(std::string) > 1u) {
		BOOST_CHECK(!piranha::alignment_check<std::string>(alignof(std::string) / 2u));
	}
#endif
}

static const std::size_t alloc_size = 2000000ull;

class custom_string: public std::string
{
	public:
		custom_string() : std::string("hello") {}
		custom_string(const custom_string &) = default;
		custom_string(custom_string &&other) noexcept : std::string(std::move(other)) {}
		template <typename... Args>
		custom_string(Args && ... params) : std::string(std::forward<Args>(params)...) {}
		custom_string &operator=(const custom_string &) = default;
		custom_string &operator=(custom_string &&other) noexcept
		{
			std::string::operator=(std::move(other));
			return *this;
		}
		~custom_string() noexcept {}
};

BOOST_AUTO_TEST_CASE(memory_parallel_perf_test)
{
	std::cout <<	"Testing int\n"
			"===========\n";
	for (unsigned i = 0u; i < settings::get_n_threads(); ++i) {
		std::cout << "n = " << i + 1u << '\n';
		boost::timer::auto_cpu_timer t;
		auto ptr1 = make_parallel_array<int>(alloc_size,i + 1u);
	}
	std::cout <<	"Testing string\n"
			"==============\n";
	for (unsigned i = 0u; i < settings::get_n_threads(); ++i) {
		std::cout << "n = " << i + 1u << '\n';
		boost::timer::auto_cpu_timer t;
		auto ptr1 = make_parallel_array<custom_string>(alloc_size,i + 1u);
	}
}
