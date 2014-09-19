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

#include "../src/thread_pool.hpp"

#define BOOST_TEST_MODULE thread_pool_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <boost/integer_traits.hpp>
#include <chrono>
#include <list>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"
#include "../src/real.hpp"
#include "../src/runtime_info.hpp"
#include "../src/thread_management.hpp"

using namespace piranha;

struct noncopyable
{
	noncopyable() = default;
	noncopyable(const noncopyable &) = delete;
	noncopyable(noncopyable &&) = delete;
	~noncopyable() = default;
	noncopyable &operator=(const noncopyable &) = delete;
	noncopyable &operator=(noncopyable &&) = delete;
};

BOOST_AUTO_TEST_CASE(thread_pool_task_queue_test)
{
	environment env;
	auto slow_task = [](){std::this_thread::sleep_for(std::chrono::milliseconds(250));};
	auto fast_task = [](int n) -> int {std::this_thread::sleep_for(std::chrono::milliseconds(1)); return n;};
	auto instant_task = [](){};
	{
	detail::task_queue tq(0);
	}
	{
	detail::task_queue tq(0);
	tq.stop();
	tq.stop();
	tq.stop();
	}
	{
	detail::task_queue tq(0);
	tq.enqueue([](){});
	tq.stop();
	tq.stop();
	}
	{
	detail::task_queue tq(0);
	tq.enqueue(slow_task);
	tq.stop();
	tq.stop();
	}
	{
	detail::task_queue tq(0);
	tq.enqueue(slow_task);
	tq.enqueue(slow_task);
	tq.enqueue(slow_task);
	}
	{
	detail::task_queue tq(0);
	auto f1 = tq.enqueue(slow_task);
	auto f2 = tq.enqueue(slow_task);
	auto f3 = tq.enqueue(slow_task);
	f3.get();
	}
	{
	detail::task_queue tq(0);
	auto f1 = tq.enqueue([](int) {throw std::runtime_error("");},1);
	BOOST_CHECK_THROW(f1.get(),std::runtime_error);
	}
	{
	detail::task_queue tq(0);
	auto f1 = tq.enqueue([](int n) {return n + n;},45);
	BOOST_CHECK(f1.get() == 90);
	}
	{
	detail::task_queue tq(0);
	using f_type = decltype(tq.enqueue(fast_task,0));
	std::list<f_type> l;
	for (int i = 0; i < 100; ++i) {
		l.push_back(tq.enqueue(fast_task,i));
	}
	tq.stop();
	int result = 0;
	for (f_type &f: l) {
		result += f.get();
	}
	BOOST_CHECK(result == 4950);
	}
	{
	detail::task_queue tq(0);
	for (int i = 0; i < 10000; ++i) {
		tq.enqueue(instant_task);
	}
	tq.stop();
	BOOST_CHECK_THROW(tq.enqueue(instant_task),std::runtime_error);
	}
	{
	detail::task_queue tq(0);
	noncopyable nc;
	tq.enqueue([](noncopyable &){},std::ref(nc));
	tq.enqueue([](const noncopyable &){},std::cref(nc));
	}
	{
	detail::task_queue tq(0);
	for (int i = 0; i < 100; ++i) {
		tq.enqueue([](){real{}.pi();});
	}
	}
#if !defined(__APPLE_CC__)
	// Check the binding.
	const unsigned hc = runtime_info::get_hardware_concurrency();
	auto bind_checker = [](unsigned n) {
		auto res = thread_management::bound_proc();
		if (!res.first || res.second != n) {
			throw std::runtime_error("");
		}
	};
	for (unsigned i = 0u; i < hc; ++i) {
		detail::task_queue tq(i);
		BOOST_CHECK_NO_THROW(tq.enqueue(bind_checker,i).get());
	}
	if (hc != 0) {
		detail::task_queue tq(hc);
		BOOST_CHECK_THROW(tq.enqueue(bind_checker,hc).get(),std::runtime_error);
	}
#endif
}

static int adder(int a, int b)
{
	return a + b;
}

BOOST_AUTO_TEST_CASE(thread_pool_test)
{
	const unsigned initial_size = thread_pool::size();
	BOOST_CHECK(initial_size > 0u);
	BOOST_CHECK(thread_pool::enqueue(0,adder,1,2).get() == 3);
	thread_pool::enqueue(0,[](){std::this_thread::sleep_for(std::chrono::milliseconds(100));});
	BOOST_CHECK(thread_pool::enqueue(0,adder,4,-5).get() == -1);
	BOOST_CHECK_THROW(thread_pool::enqueue(initial_size,adder,4,-5),std::invalid_argument);
#if !defined(__APPLE_CC__)
	BOOST_CHECK(thread_pool::enqueue(0,[](){return thread_management::bound_proc();}).get() == std::make_pair(true,0u));
#endif
	BOOST_CHECK_THROW(thread_pool::enqueue(0,[](){throw std::runtime_error("");}).get(),std::runtime_error);
	auto fast_task = [](int n) -> int {std::this_thread::sleep_for(std::chrono::milliseconds(1)); return n;};
	for (unsigned i = 0u; i < initial_size; ++i) {
		for (int n = 0; n < 1000; ++n) {
			thread_pool::enqueue(i,fast_task,n);
		}
	}
	for (unsigned i = 0u; i < initial_size; ++i) {
		thread_pool::enqueue(i,[](){}).get();
	}
	auto slow_task = [](){std::this_thread::sleep_for(std::chrono::milliseconds(250));};
	thread_pool::resize(1);
	thread_pool::enqueue(0,slow_task);
	thread_pool::resize(20u);
	BOOST_CHECK(thread_pool::size() == 20u);
	for (unsigned i = 0u; i < 20u; ++i) {
		thread_pool::enqueue(0u,slow_task);
		for (int n = 1; n < 1000; ++n) {
			thread_pool::enqueue(i,fast_task,n);
		}
	}
	BOOST_CHECK(thread_pool::size() == 20u);
	thread_pool::resize(10u);
	BOOST_CHECK(thread_pool::size() == 10u);
#if !defined(__APPLE_CC__)
	if (initial_size != boost::integer_traits<unsigned>::const_max) {
		thread_pool::resize(initial_size + 1u);
		auto func = [](){return thread_management::bound_proc();};
		std::vector<decltype(thread_pool::enqueue(0u,func))> list;
		for (unsigned i = 0; i < initial_size + 1u; ++i) {
			list.push_back(thread_pool::enqueue(i,func));
		}
		std::all_of(list.begin(),list.begin() + initial_size,[](decltype(thread_pool::enqueue(0u,func)) &f){return f.get().first;});
		BOOST_CHECK(!(list.begin() + initial_size)->get().second);
	}
#endif
	BOOST_CHECK_THROW(thread_pool::resize(0u),std::invalid_argument);
	BOOST_CHECK(thread_pool::size() != 0u);
}

BOOST_AUTO_TEST_CASE(thread_pool_future_list_test)
{
	thread_pool::resize(10u);
	auto null_task = [](){};
	future_list<decltype(thread_pool::enqueue(0,null_task))> f1;
	f1.wait_all();
	f1.wait_all();
	f1.get_all();
	f1.get_all();
	auto fast_task = [](){std::this_thread::sleep_for(std::chrono::milliseconds(1));};
	future_list<decltype(thread_pool::enqueue(0,fast_task))> f2;
	for (unsigned i = 0u; i < 10u; ++i) {
		for (unsigned j = 0u; j < 100u; ++j) {
			f2.push_back(thread_pool::enqueue(i,fast_task));
		}
	}
	f2.wait_all();
	f2.wait_all();
	f2.get_all();
	f2.get_all();
	auto thrower = [](){throw std::runtime_error("");};
	future_list<decltype(thread_pool::enqueue(0,thrower))> f3;
	for (unsigned i = 0u; i < 10u; ++i) {
		for (unsigned j = 0u; j < 100u; ++j) {
			f3.push_back(thread_pool::enqueue(i,thrower));
		}
	}
	f3.wait_all();
	f3.wait_all();
	BOOST_CHECK_THROW(f3.get_all(),std::runtime_error);
	BOOST_CHECK_THROW(f3.get_all(),std::runtime_error);
	BOOST_CHECK_THROW(f3.get_all(),std::runtime_error);
	// Try with empty futures.
	future_list<decltype(thread_pool::enqueue(0,thrower))> f4;
	for (unsigned i = 0u; i < 100u; ++i) {
		f4.push_back(decltype(thread_pool::enqueue(0,thrower))());
	}
	f4.wait_all();
	f4.wait_all();
	f4.get_all();
	f4.get_all();
}

BOOST_AUTO_TEST_CASE(thread_pool_use_threads_test)
{
	thread_pool::resize(4u);
	BOOST_CHECK(thread_pool::use_threads(100u,3u) == 4u);
	BOOST_CHECK_THROW(thread_pool::use_threads(100u,0u),std::invalid_argument);
	BOOST_CHECK(thread_pool::use_threads(100u,30u) == 3u);
	auto f1 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(100u,3u);
	});
	auto f2 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(100u,1u);
	});
	auto f3 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(100u,0u);
	});
	BOOST_CHECK_EQUAL(f1.get(),1u);
	BOOST_CHECK_EQUAL(f2.get(),1u);
	BOOST_CHECK_THROW(f3.get(),std::invalid_argument);
	thread_pool::resize(1u);
	BOOST_CHECK(thread_pool::use_threads(100u,3u) == 1u);
	BOOST_CHECK_THROW(thread_pool::use_threads(100u,0u),std::invalid_argument);
	BOOST_CHECK(thread_pool::use_threads(100u,30u) == 1u);
	auto f4 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(100u,3u);
	});
	auto f5 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(100u,1u);
	});
	auto f6 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(100u,0u);
	});
	BOOST_CHECK_EQUAL(f4.get(),1u);
	BOOST_CHECK_EQUAL(f5.get(),1u);
	BOOST_CHECK_THROW(f6.get(),std::invalid_argument);
	thread_pool::resize(4u);
	BOOST_CHECK(thread_pool::use_threads(integer(100u),integer(3u)) == 4u);
	BOOST_CHECK_THROW(thread_pool::use_threads(integer(100u),integer(0u)),std::invalid_argument);
	BOOST_CHECK(thread_pool::use_threads(integer(100u),integer(30u)) == 3u);
	auto f7 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(integer(100u),integer(3u));
	});
	auto f8 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(integer(100u),integer(1u));
	});
	auto f9 = thread_pool::enqueue(0u,[]() {
		return thread_pool::use_threads(integer(100u),integer(0u));
	});
	BOOST_CHECK_EQUAL(f7.get(),1u);
	BOOST_CHECK_EQUAL(f8.get(),1u);
	BOOST_CHECK_THROW(f9.get(),std::invalid_argument);
}
