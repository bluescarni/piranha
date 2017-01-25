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

#ifndef PIRANHA_SETTINGS_HPP
#define PIRANHA_SETTINGS_HPP

#include <atomic>
#include <mutex>
#include <stdexcept>

#include <piranha/config.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/runtime_info.hpp>
#include <piranha/thread_pool.hpp>

namespace piranha
{

namespace detail
{

template <typename = int>
struct base_settings {
    static std::mutex m_mutex;
    static unsigned m_cache_line_size;
    static unsigned long m_max_term_output;
    static const unsigned long m_default_max_term_output = 20ul;
    static std::atomic_ullong s_min_work_per_thread;
    // NOTE: this corresponds to circa 2% overhead from thread management on a common desktop
    // machine around 2012 for the fastest series multiplication scenario.
    static const unsigned long long s_default_min_work_per_thread = 250000ull;
};

template <typename T>
std::mutex base_settings<T>::m_mutex;

template <typename T>
unsigned base_settings<T>::m_cache_line_size = runtime_info::get_cache_line_size();

template <typename T>
unsigned long base_settings<T>::m_max_term_output = base_settings<T>::m_default_max_term_output;

template <typename T>
const unsigned long base_settings<T>::m_default_max_term_output;

template <typename T>
const unsigned long long base_settings<T>::s_default_min_work_per_thread;

template <typename T>
std::atomic_ullong base_settings<T>::s_min_work_per_thread(base_settings<T>::s_default_min_work_per_thread);
}

/// Global settings.
/**
 * \note
 * The template parameter in this class is unused: its only purpose is to prevent the instantiation
 * of the class' methods if they are not explicitly used. Client code should always employ the
 * piranha::settings alias.
 *
 * This class stores the global settings of piranha's runtime environment.
 * The methods of this class are thread-safe.
 */
template <typename = void>
class settings_ : private detail::base_settings<>
{
public:
    /// Get the number of threads available for use by piranha.
    /**
     * The initial value is set to the maximum between 1 and piranha::runtime_info::get_hardware_concurrency().
     * This function is equivalent to piranha::thread_pool::size().
     *
     * @return the number of threads that will be available for use by piranha.
     *
     * @throws unspecified any exception thrown by piranha::thread_pool::size().
     */
    static unsigned get_n_threads()
    {
        return thread_pool::size();
    }
    /// Set the number of threads available for use by piranha.
    /**
     * This function is equivalent to piranha::thread_pool::resize().
     *
     * @param n the desired number of threads.
     *
     * @throws unspecified any exception thrown by piranha::thread_pool::resize().
     */
    static void set_n_threads(unsigned n)
    {
        thread_pool::resize(n);
    }
    /// Reset the number of threads available for use by piranha.
    /**
     * Will set the number of threads to the maximum between 1 and piranha::runtime_info::get_hardware_concurrency().
     *
     * @throws unspecified any exception thrown by set_n_threads().
     */
    static void reset_n_threads()
    {
        const auto candidate = runtime_info::get_hardware_concurrency();
        set_n_threads((candidate > 0u) ? candidate : 1u);
    }
    /// Set the thread binding policy.
    /**
     * This method is an alias for piranha::thread_pool::set_binding(). If \p flag is \p true, each
     * thread used by piranha will be bound to a different processor/core. If \p flag is \p false,
     * this method will unbind piranha's threads from any processor/core to which they might be bound.
     *
     * By default piranha's threads are not bound to any specific processor/core.
     *
     * @param flag the desired thread binding policy.
     *
     * @throws unspecified any exception thrown by piranha::thread_pool::set_binding().
     */
    static void set_thread_binding(bool flag)
    {
        thread_pool::set_binding(flag);
    }
    /// Get the thread binding policy.
    /**
     * This method is an alias for piranha::thread_pool::get_binding(). It will return the flag set by
     * settings::set_thread_binding() (which is \p false by default on program startup).
     *
     * @return the active thread binding policy.
     *
     * @throws unspecified any exception thrown by piranha::thread_pool::get_binding().
     */
    static bool get_thread_binding()
    {
        return thread_pool::get_binding();
    }
    /// Get the cache line size.
    /**
     * The initial value is set to the output of piranha::runtime_info::get_cache_line_size(). The value
     * can be overridden with set_cache_line_size() in case the detection fails and the value is set to zero.
     *
     * @return data cache line size (in bytes).
     *
     * @throws std::system_error in case of failure(s) by threading primitives.
     */
    static unsigned get_cache_line_size()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache_line_size;
    }
    /// Set the cache line size.
    /**
     * Overrides the detected cache line size. This method should be used only if the automatic
     * detection fails.
     *
     * @param n data cache line size (in bytes).
     *
     * @throws std::system_error in case of failure(s) by threading primitives.
     */
    static void set_cache_line_size(unsigned n)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache_line_size = n;
    }
    /// Reset the cache line size.
    /**
     * Will set the value to the output of piranha::runtime_info::get_cache_line_size().
     *
     * @throws std::system_error in case of failure(s) by threading primitives.
     */
    static void reset_cache_line_size()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache_line_size = runtime_info::get_cache_line_size();
    }
    /// Get max term output.
    /**
     * @return maximum number of terms displayed when printing series.
     *
     * @throws std::system_error in case of failure(s) by threading primitives.
     */
    static unsigned long get_max_term_output()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_max_term_output;
    }
    /// Set max term output.
    /**
     * @param n maximum number of terms to be displayed when printing series.
     *
     * @throws std::system_error in case of failure(s) by threading primitives.
     */
    static void set_max_term_output(unsigned long n)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_max_term_output = n;
    }
    /// Reset max term output.
    /**
     * Will set the max term output value to the default.
     *
     * @throws std::system_error in case of failure(s) by threading primitives.
     */
    static void reset_max_term_output()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_max_term_output = m_default_max_term_output;
    }
    /// Get the minimum work per thread.
    /**
     * @return the minimum work per thread.
     */
    static unsigned long long get_min_work_per_thread()
    {
        return s_min_work_per_thread.load();
    }
    /// Set the minimum work per thread.
    /**
     * @param n the minimum work per thread.
     *
     * @throws std::invalid_argument if \n is zero.
     */
    static void set_min_work_per_thread(unsigned long long n)
    {
        if (unlikely(n == 0u)) {
            piranha_throw(std::invalid_argument, "the minimum work per thread value must be strictly positive");
        }
        return s_min_work_per_thread.store(n);
    }
    /// Reset the minimum work per thread.
    /**
     * The value will be reset to the default initial value.
     */
    static void reset_min_work_per_thread()
    {
        s_min_work_per_thread.store(s_default_min_work_per_thread);
    }
};

/// Alias for piranha::settings_.
/**
 * This is the alias through which the methods in piranha::settings_ should be called.
 */
using settings = settings_<>;
}

#endif
