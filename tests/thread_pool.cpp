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
#include <chrono>
#include <list>
#include <stdexcept>
#include <thread>

#include "../src/environment.hpp"
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
	detail::task_queue tq;
	}
	{
	detail::task_queue tq;
	tq.stop();
	tq.stop();
	tq.stop();
	}
	{
	detail::task_queue tq;
	tq.enqueue([](){});
	tq.stop();
	tq.stop();
	}
	{
	detail::task_queue tq;
	tq.enqueue(slow_task);
	tq.stop();
	tq.stop();
	}
	{
	detail::task_queue tq;
	tq.enqueue(slow_task);
	tq.enqueue(slow_task);
	tq.enqueue(slow_task);
	}
	{
	detail::task_queue tq;
	auto f1 = tq.enqueue(slow_task);
	auto f2 = tq.enqueue(slow_task);
	auto f3 = tq.enqueue(slow_task);
	f3.get();
	}
	{
	detail::task_queue tq;
	auto f1 = tq.enqueue([](int) {throw std::runtime_error("");},1);
	BOOST_CHECK_THROW(f1.get(),std::runtime_error);
	}
	{
	detail::task_queue tq;
	auto f1 = tq.enqueue([](int n) {return n + n;},45);
	BOOST_CHECK(f1.get() == 90);
	}
	{
	detail::task_queue tq;
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
	detail::task_queue tq;
	for (int i = 0; i < 10000; ++i) {
		tq.enqueue(instant_task);
	}
	tq.stop();
	BOOST_CHECK_THROW(tq.enqueue(instant_task),std::runtime_error);
	}
	{
	detail::task_queue tq;
	noncopyable nc;
	tq.enqueue([](noncopyable &){},std::ref(nc));
	tq.enqueue([](const noncopyable &){},std::cref(nc));
	}
	{
	detail::task_queue tq;
	for (int i = 0; i < 100; ++i) {
		tq.enqueue([](){real{}.pi();});
	}
	}
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
}

BOOST_AUTO_TEST_CASE(thread_pool_test)
{
	if (thread_pool::size() != 0u) {
		thread_pool::enqueue(0,[](){});
	}
}
