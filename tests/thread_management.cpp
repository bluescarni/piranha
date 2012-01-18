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

#include "../src/thread_management.hpp"

#define BOOST_TEST_MODULE thread_management_test
#include <boost/test/unit_test.hpp>

#include "../src/runtime_info.hpp"
#include "../src/settings.hpp"
#include "../src/task_group.hpp"
#include "../src/thread_barrier.hpp"
#include "../src/threading.hpp"

// TODO: check for exceptions throwing.

piranha::mutex mutex;

static inline void test_function()
{
	for (unsigned i = 0; i < piranha::runtime_info::get_hardware_concurrency(); ++i) {
		piranha::thread_management::bind_to_proc(i);
		const auto retval = piranha::thread_management::bound_proc();
		// Lock because Boost unit test is not thread-safe.
		piranha::lock_guard<piranha::mutex>::type lock(mutex);
		BOOST_CHECK_EQUAL(retval.first,true);
		BOOST_CHECK_EQUAL(retval.second,i);
	}
}

// Check binding on new threads thread.
BOOST_AUTO_TEST_CASE(thread_management_new_threads_bind)
{
	for (unsigned i = 0; i < piranha::runtime_info::get_hardware_concurrency(); ++i) {
		piranha::thread t(test_function);
		t.join();
	}
}

// Check thread-safe binding using a thread group.
BOOST_AUTO_TEST_CASE(thread_management_task_group_bind)
{
	piranha::task_group tg;
	for (unsigned i = 0; i < piranha::runtime_info::get_hardware_concurrency(); ++i) {
		tg.add_task(test_function);
	}
	tg.wait_all();
}

// binder tests.
BOOST_AUTO_TEST_CASE(thread_management_binder)
{
	// Check we are not binding from main thread.
	piranha::thread_management::binder b;
	BOOST_CHECK_EQUAL(false,piranha::thread_management::bound_proc().first);
	unsigned hc = piranha::runtime_info::get_hardware_concurrency();
	for (unsigned i = 0; i < hc; ++i) {
		piranha::task_group tg;
		for (unsigned j = 0; j < i; ++j) {
			auto f = []() -> void {
				piranha::thread_management::binder b;
				piranha::lock_guard<piranha::mutex>::type lock(mutex);
				BOOST_CHECK_EQUAL(true,piranha::thread_management::bound_proc().first);
			};
			tg.add_task(f);
		}
		tg.wait_all();
	}
	// The following tests make sense only if we can detect hardware_concurrency.
	if (!hc) {
		return;
	}
	unsigned count = 0;
	piranha::settings::set_n_threads(hc + 1);
	piranha::task_group tg;
	piranha::thread_barrier tb(hc + 1);
	for (unsigned i = 0; i < hc + 1; ++i) {
		auto f = [&count,&tb,hc]() -> void {
			piranha::unique_lock<piranha::mutex>::type lock(mutex);
			piranha::thread_management::binder b;
			if (count >= hc) {
				BOOST_CHECK_EQUAL(false,piranha::thread_management::bound_proc().first);
			} else {
				BOOST_CHECK_EQUAL(true,piranha::thread_management::bound_proc().first);
			}
			++count;
			lock.unlock();
			tb.wait();
		};
		tg.add_task(f);
	}
	tg.wait_all();
}

// Check binding on main thread. Do it last so we do not bind the main thread for the other tests.
BOOST_AUTO_TEST_CASE(thread_management_main_thread_bind)
{
	test_function();
}
