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

#include <piranha/thread_pool.hpp>

#define BOOST_TEST_MODULE thread_pool_test
#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <chrono>
#include <limits>
#include <list>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <piranha/init.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/real.hpp>
#include <piranha/runtime_info.hpp>
#include <piranha/thread_management.hpp>
#include <piranha/type_traits.hpp>

using namespace piranha;

// Typedef for detection idiom.
template <typename F, typename... Args>
using enqueue_t = decltype(thread_pool::enqueue(0u, std::declval<F>(), std::declval<Args>()...));

struct noncopyable {
    noncopyable() = default;
    noncopyable(const noncopyable &) = delete;
    noncopyable(noncopyable &&) = delete;
    ~noncopyable() = default;
    noncopyable &operator=(const noncopyable &) = delete;
    noncopyable &operator=(noncopyable &&) = delete;
};

struct noncopyable_functor {
    noncopyable_functor() = default;
    noncopyable_functor(const noncopyable_functor &) = delete;
    noncopyable_functor(noncopyable_functor &&) = delete;
    ~noncopyable_functor() = default;
    noncopyable_functor &operator=(const noncopyable_functor &) = delete;
    noncopyable_functor &operator=(noncopyable_functor &&) = delete;
    void operator()() const;
};

noncopyable noncopyable_ret_f();

void requires_move(int &&);

static int nn = 5;

struct ref_test_functor {
    template <typename T>
    void operator()(T &arg)
    {
        BOOST_CHECK((std::is_same<T, int>::value));
        BOOST_CHECK(&arg == &nn);
    }
};

struct cref_test_functor {
    template <typename T>
    void operator()(T &arg)
    {
        BOOST_CHECK((std::is_same<T, const int>::value));
        BOOST_CHECK(&arg == &nn);
    }
};

BOOST_AUTO_TEST_CASE(thread_pool_task_queue_test)
{
    init();
    // A few simple tests.
    auto simple_00 = []() {};
    BOOST_CHECK((is_detected<enqueue_t, decltype(simple_00)>::value));
    BOOST_CHECK((is_detected<enqueue_t, decltype(simple_00) &>::value));
    BOOST_CHECK((is_detected<enqueue_t, const decltype(simple_00) &>::value));
    BOOST_CHECK((is_detected<enqueue_t, const decltype(simple_00)>::value));
    BOOST_CHECK((!is_detected<enqueue_t, decltype(simple_00) &, int>::value));
    BOOST_CHECK((!is_detected<enqueue_t, decltype(simple_00) &, void>::value));
    BOOST_CHECK((!is_detected<enqueue_t, void, int>::value));
    BOOST_CHECK((!is_detected<enqueue_t, decltype(simple_00) &, void>::value));
    // Check that noncopyable functor disables enqueue.
    BOOST_CHECK((!is_detected<enqueue_t, noncopyable_functor &>::value));
    BOOST_CHECK((!is_detected<enqueue_t, noncopyable_functor &&>::value));
    BOOST_CHECK((!is_detected<enqueue_t, noncopyable_functor>::value));
    // Check that noncopyable return type disables enqueue.
    BOOST_CHECK((!is_detected<enqueue_t, decltype(noncopyable_ret_f)>::value));
    // Check that if a function argument must be passed as rvalue ref, then enqueue is disabled.
    BOOST_CHECK((!is_detected<enqueue_t, decltype(requires_move), int>::value));
    // Test that std::ref/cref works as intended (i.e., it does not copy the value) and that
    // reference_wrapper is removed when the argument is passed to the functor.
    auto fut_ref = thread_pool::enqueue(0, ref_test_functor{}, std::ref(nn));
    fut_ref.get();
    fut_ref = thread_pool::enqueue(0, cref_test_functor{}, std::cref(nn));
    fut_ref.get();
    auto slow_task = []() { std::this_thread::sleep_for(std::chrono::milliseconds(250)); };
    auto fast_task = [](int n) -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return n;
    };
    auto instant_task = []() noexcept {};
    {
        task_queue tq(0, true);
    }
    {
        task_queue tq(0, true);
        tq.stop();
        tq.stop();
        tq.stop();
    }
    {
        task_queue tq(0, true);
        tq.enqueue([]() noexcept {});
        tq.stop();
        tq.stop();
    }
    {
        task_queue tq(0, true);
        tq.enqueue(slow_task);
        tq.stop();
        tq.stop();
    }
    {
        task_queue tq(0, true);
        tq.enqueue(slow_task);
        tq.enqueue(slow_task);
        tq.enqueue(slow_task);
    }
    {
        task_queue tq(0, true);
        auto f1 = tq.enqueue(slow_task);
        auto f2 = tq.enqueue(slow_task);
        auto f3 = tq.enqueue(slow_task);
        f3.get();
    }
    {
        task_queue tq(0, true);
        auto f1 = tq.enqueue([](int) { throw std::runtime_error(""); }, 1);
        BOOST_CHECK_THROW(f1.get(), std::runtime_error);
    }
    {
        task_queue tq(0, true);
        auto f1 = tq.enqueue([](int n) noexcept { return n + n; }, 45);
        BOOST_CHECK(f1.get() == 90);
    }
    {
        task_queue tq(0, true);
        using f_type = decltype(tq.enqueue(fast_task, 0));
        std::list<f_type> l;
        for (int i = 0; i < 100; ++i) {
            l.push_back(tq.enqueue(fast_task, i));
        }
        tq.stop();
        int result = 0;
        for (f_type &f : l) {
            result += f.get();
        }
        BOOST_CHECK(result == 4950);
    }
    {
        task_queue tq(0, true);
        for (int i = 0; i < 10000; ++i) {
            tq.enqueue(instant_task);
        }
        tq.stop();
        BOOST_CHECK_EXCEPTION(tq.enqueue(instant_task), std::runtime_error, [](const std::runtime_error &e) {
            return boost::contains(e.what(), "cannot enqueue task while the task queue is stopping");
        });
    }
    {
        task_queue tq(0, true);
        noncopyable nc;
        tq.enqueue([](noncopyable &) noexcept {}, std::ref(nc));
        tq.enqueue([](const noncopyable &) noexcept {}, std::cref(nc));
    }
    {
        task_queue tq(0, true);
        for (int i = 0; i < 100; ++i) {
            tq.enqueue([]() { real{}.pi(); });
        }
    }
#if !defined(__APPLE_CC__)
    // Check the binding.
    const unsigned hc = runtime_info::get_hardware_concurrency();
    auto bind_checker = [](unsigned n) {
        auto res = bound_proc();
        if (!res.first || res.second != n) {
            throw std::runtime_error("");
        }
    };
    for (unsigned i = 0u; i < hc; ++i) {
        task_queue tq(i, true);
        BOOST_CHECK_NO_THROW(tq.enqueue(bind_checker, i).get());
    }
    if (hc != 0) {
        task_queue tq(hc, true);
        BOOST_CHECK_THROW(tq.enqueue(bind_checker, hc).get(), std::runtime_error);
    }
    auto unbound_checker = []() {
        auto res = bound_proc();
        if (res.first) {
            throw std::runtime_error("");
        }
    };
    for (unsigned i = 0u; i < hc; ++i) {
        task_queue tq(i, false);
        BOOST_CHECK_NO_THROW(tq.enqueue(unbound_checker).get());
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
    BOOST_CHECK_EQUAL(thread_pool::get_binding(), false);
    BOOST_CHECK(thread_pool::enqueue(0, adder, 1, 2).get() == 3);
    thread_pool::enqueue(0, []() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
    BOOST_CHECK(thread_pool::enqueue(0, adder, 4, -5).get() == -1);
    BOOST_CHECK_EXCEPTION(
        thread_pool::enqueue(initial_size, adder, 4, -5), std::invalid_argument,
        [](const std::invalid_argument &e) { return boost::contains(e.what(), "the thread pool contains only "); });
#if !defined(__APPLE_CC__)
    BOOST_CHECK(thread_pool::enqueue(0, []() { return bound_proc(); }).get().first == false);
    thread_pool::set_binding(true);
    BOOST_CHECK(thread_pool::enqueue(0, []() { return bound_proc(); }).get() == std::make_pair(true, 0u));
    BOOST_CHECK_EQUAL(thread_pool::get_binding(), true);
    thread_pool::set_binding(false);
    BOOST_CHECK_EQUAL(thread_pool::get_binding(), false);
#endif
    BOOST_CHECK_THROW(thread_pool::enqueue(0, []() { throw std::runtime_error(""); }).get(), std::runtime_error);
    auto fast_task = [](int n) -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return n;
    };
    for (unsigned i = 0u; i < initial_size; ++i) {
        for (int n = 0; n < 1000; ++n) {
            thread_pool::enqueue(i, fast_task, n);
        }
    }
    for (unsigned i = 0u; i < initial_size; ++i) {
        thread_pool::enqueue(i, []() noexcept {}).get();
    }
    auto slow_task = []() { std::this_thread::sleep_for(std::chrono::milliseconds(250)); };
    thread_pool::resize(1);
    BOOST_CHECK_EQUAL(thread_pool::get_binding(), false);
    thread_pool::enqueue(0, slow_task);
    thread_pool::resize(20u);
    BOOST_CHECK_EQUAL(thread_pool::get_binding(), false);
    BOOST_CHECK(thread_pool::size() == 20u);
    thread_pool::set_binding(true);
    thread_pool::set_binding(true);
    thread_pool::resize(1);
    BOOST_CHECK_EQUAL(thread_pool::get_binding(), true);
    thread_pool::enqueue(0, slow_task);
    thread_pool::resize(20u);
    BOOST_CHECK_EQUAL(thread_pool::get_binding(), true);
    BOOST_CHECK(thread_pool::size() == 20u);
    thread_pool::set_binding(false);
    thread_pool::set_binding(false);
    for (unsigned i = 0u; i < 20u; ++i) {
        thread_pool::enqueue(0u, slow_task);
        for (int n = 1; n < 1000; ++n) {
            thread_pool::enqueue(i, fast_task, n);
        }
    }
    BOOST_CHECK(thread_pool::size() == 20u);
    thread_pool::resize(10u);
    BOOST_CHECK(thread_pool::size() == 10u);
#if !defined(__APPLE_CC__)
    if (initial_size != std::numeric_limits<unsigned>::max()) {
        thread_pool::resize(initial_size + 1u);
        auto func = []() { return bound_proc(); };
        std::vector<decltype(thread_pool::enqueue(0u, func))> list;
        for (unsigned i = 0; i < initial_size + 1u; ++i) {
            list.push_back(thread_pool::enqueue(i, func));
        }
        std::all_of(list.begin(), list.begin() + initial_size,
                    [](decltype(thread_pool::enqueue(0u, func)) &f) { return f.get().first; });
        BOOST_CHECK(!(list.begin() + initial_size)->get().second);
    }
#endif
    BOOST_CHECK_EXCEPTION(thread_pool::resize(0u), std::invalid_argument, [](const std::invalid_argument &e) {
        return boost::contains(e.what(), "cannot resize the thread pool to zero");
    });
    BOOST_CHECK(thread_pool::size() != 0u);
}

BOOST_AUTO_TEST_CASE(thread_pool_future_list_test)
{
    thread_pool::resize(10u);
    auto null_task = []() {};
    future_list<decltype(null_task())> f1;
    BOOST_CHECK(!std::is_copy_assignable<decltype(f1)>::value);
    BOOST_CHECK(!std::is_move_assignable<decltype(f1)>::value);
    f1.wait_all();
    f1.wait_all();
    f1.get_all();
    f1.get_all();
    auto fast_task = []() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); };
    future_list<decltype(fast_task())> f2;
    for (unsigned i = 0u; i < 10u; ++i) {
        for (unsigned j = 0u; j < 100u; ++j) {
            f2.push_back(thread_pool::enqueue(i, fast_task));
        }
    }
    f2.wait_all();
    f2.wait_all();
    f2.get_all();
    f2.get_all();
    auto thrower = []() { throw std::runtime_error(""); };
    future_list<decltype(thrower())> f3;
    for (unsigned i = 0u; i < 10u; ++i) {
        for (unsigned j = 0u; j < 100u; ++j) {
            f3.push_back(thread_pool::enqueue(i, thrower));
        }
    }
    f3.wait_all();
    f3.wait_all();
    BOOST_CHECK_THROW(f3.get_all(), std::runtime_error);
    BOOST_CHECK_THROW(f3.get_all(), std::runtime_error);
    BOOST_CHECK_THROW(f3.get_all(), std::runtime_error);
    // Try with empty futures.
    future_list<decltype(thrower())> f4;
    for (unsigned i = 0u; i < 100u; ++i) {
        f4.push_back(decltype(thread_pool::enqueue(0, thrower))());
    }
    f4.wait_all();
    f4.wait_all();
    f4.get_all();
    f4.get_all();
}

BOOST_AUTO_TEST_CASE(thread_pool_use_threads_test)
{
    thread_pool::resize(4u);
    BOOST_CHECK(thread_pool::use_threads(100u, 3u) == 4u);
    BOOST_CHECK_EXCEPTION(
        thread_pool::use_threads(100u, 0u), std::invalid_argument, [](const std::invalid_argument &e) {
            return boost::contains(e.what(),
                                   "invalid value of 0 for minimum work per thread (it must be strictly positive)");
        });
    BOOST_CHECK_EXCEPTION(
        thread_pool::use_threads(0u, 100u), std::invalid_argument, [](const std::invalid_argument &e) {
            return boost::contains(e.what(), "invalid value of 0 for work size (it must be strictly positive)");
        });
    BOOST_CHECK_THROW(thread_pool::use_threads(0u, 0u), std::invalid_argument);
    BOOST_CHECK_THROW(thread_pool::use_threads(100_z, 0_z), std::invalid_argument);
    BOOST_CHECK_THROW(thread_pool::use_threads(0_z, 100_z), std::invalid_argument);
    BOOST_CHECK_THROW(thread_pool::use_threads(0_z, 0_z), std::invalid_argument);
    BOOST_CHECK_THROW(thread_pool::use_threads(100_z, -1_z), std::invalid_argument);
    BOOST_CHECK_THROW(thread_pool::use_threads(-1_z, 100_z), std::invalid_argument);
    BOOST_CHECK_THROW(thread_pool::use_threads(-1_z, -1_z), std::invalid_argument);
    BOOST_CHECK(thread_pool::use_threads(100u, 30u) == 3u);
    auto f1 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(100u, 3u); });
    auto f2 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(100u, 1u); });
    auto f3 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(100u, 0u); });
    BOOST_CHECK_EQUAL(f1.get(), 1u);
    BOOST_CHECK_EQUAL(f2.get(), 1u);
    BOOST_CHECK_THROW(f3.get(), std::invalid_argument);
    thread_pool::resize(1u);
    BOOST_CHECK(thread_pool::use_threads(100u, 3u) == 1u);
    BOOST_CHECK_THROW(thread_pool::use_threads(100u, 0u), std::invalid_argument);
    BOOST_CHECK(thread_pool::use_threads(100u, 30u) == 1u);
    auto f4 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(100u, 3u); });
    auto f5 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(100u, 1u); });
    auto f6 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(100u, 0u); });
    BOOST_CHECK_EQUAL(f4.get(), 1u);
    BOOST_CHECK_EQUAL(f5.get(), 1u);
    BOOST_CHECK_THROW(f6.get(), std::invalid_argument);
    thread_pool::resize(4u);
    BOOST_CHECK(thread_pool::use_threads(integer(100u), integer(3u)) == 4u);
    BOOST_CHECK_THROW(thread_pool::use_threads(integer(100u), integer(0u)), std::invalid_argument);
    BOOST_CHECK(thread_pool::use_threads(integer(100u), integer(30u)) == 3u);
    auto f7 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(integer(100u), integer(3u)); });
    auto f8 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(integer(100u), integer(1u)); });
    auto f9 = thread_pool::enqueue(0u, []() { return thread_pool::use_threads(integer(100u), integer(0u)); });
    BOOST_CHECK_EQUAL(f7.get(), 1u);
    BOOST_CHECK_EQUAL(f8.get(), 1u);
    BOOST_CHECK_THROW(f9.get(), std::invalid_argument);
}
