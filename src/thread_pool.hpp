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

#ifndef PIRANHA_THREAD_POOL_HPP
#define PIRANHA_THREAD_POOL_HPP

#include <algorithm>
#include <atomic>
#include <boost/lexical_cast.hpp>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <future>
// See old usage of cout below.
// #include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "config.hpp"
#include "detail/atomic_lock_guard.hpp"
#include "detail/mpfr.hpp"
#include "exceptions.hpp"
#include "mp_integer.hpp"
#include "runtime_info.hpp"
#include "thread_management.hpp"
#include "type_traits.hpp"

namespace piranha
{

inline namespace impl
{

// Task queue class. Inspired by:
// https://github.com/progschj/ThreadPool
struct task_queue {
    struct runner {
        runner(task_queue *ptr, unsigned n, bool bind) : m_ptr(ptr), m_n(n), m_bind(bind)
        {
        }
        void operator()() const
        {
            if (m_bind) {
                try {
                    bind_to_proc(m_n);
                } catch (...) {
                    // Don't stop if we cannot bind.
                    // NOTE: logging candidate.
                }
            }
            try {
                while (true) {
                    std::unique_lock<std::mutex> lock(m_ptr->m_mutex);
                    while (!m_ptr->m_stop && m_ptr->m_tasks.empty()) {
                        // Need to wait for something to happen only if the task
                        // list is empty and we are not stopping.
                        // NOTE: wait will be noexcept in C++14.
                        m_ptr->m_cond.wait(lock);
                    }
                    if (m_ptr->m_stop && m_ptr->m_tasks.empty()) {
                        // If the stop flag was set, and we do not have more tasks,
                        // just exit.
                        break;
                    }
                    // NOTE: move constructor of std::function could throw, unfortunately.
                    std::function<void()> task(std::move(m_ptr->m_tasks.front()));
                    m_ptr->m_tasks.pop();
                    lock.unlock();
                    task();
                }
            } catch (...) {
                // The errors we could get here are:
                // - threading primitives,
                // - move-construction of std::function,
                // - queue popping (I guess unlikely, as the destructor of std::function
                //   is noexcept).
                // In any case, not much that can be done to recover from this, better to abort.
                // NOTE: logging candidate.
                std::abort();
            }
            // Free the MPFR caches.
            ::mpfr_free_cache();
        }
        task_queue *m_ptr;
        const unsigned m_n;
        const bool m_bind;
    };

    task_queue(unsigned n, bool bind) : m_stop(false)
    {
        // NOTE: this seems to be ok wrt order of evaluation, since the ctor of runner cannot throw.
        // In general, it could happen that:
        // - new allocates,
        // - runner is constructed and throws,
        // - the memory allocated by new is not freed.
        // See the classic: http://gotw.ca/gotw/056.htm
        m_thread.reset(new std::thread(runner{this, n, bind}));
    }
    ~task_queue()
    {
        // NOTE: logging candidate (catch any exception,
        // log it and abort as there is not much we can do).
        try {
            stop();
        } catch (...) {
            std::abort();
        }
    }
    // Small utility to remove reference_wrapper.
    template <typename T>
    struct unwrap_ref {
        using type = T;
    };
    template <typename T>
    struct unwrap_ref<std::reference_wrapper<T>> {
        using type = T;
    };
    template <typename T>
    using unwrap_ref_t = typename unwrap_ref<T>::type;
    // NOTE: the functor F will be forwarded to std::bind in order to create a nullary wrapper. The nullary wrapper
    // will create copies of the input arguments, and it will then pass these copies as lvalue refs to a copy of the
    // original functor when the call operator is invoked (with special handling of reference wrappers). Thus, the
    // real invocation of F is not simply F(args), but this more complicated type below.
    // NOTE: this is one place where it seems we really want decay instead of uncvref, as decay is applied
    // also by std::bind() to F.
    template <typename F, typename... Args>
    using f_ret_type = decltype(std::declval<decay_t<F> &>()(std::declval<unwrap_ref_t<uncvref_t<Args>> &>()...));
    // enqueue() will be enabled if:
    // - f_ret_type is a valid type (checked in the return type),
    // - we can construct the nullary wrapper via std::bind() (this requires F and Args to be ctible from the input
    //   arguments),
    // - we can build a packaged_task from the nullary wrapper (requires F and Args to be move/copy ctible),
    // - the return type of F is returnable.
    template <typename F, typename... Args>
    using enabler
        = enable_if_t<conjunction<std::is_constructible<decay_t<F>, F>, std::is_constructible<uncvref_t<Args>, Args>...,
                                  disjunction<std::is_copy_constructible<decay_t<F>>,
                                              std::is_move_constructible<decay_t<F>>>,
                                  conjunction<disjunction<std::is_copy_constructible<uncvref_t<Args>>,
                                                          std::is_move_constructible<uncvref_t<Args>>>>...,
                                  is_returnable<f_ret_type<F, Args...>>>::value,
                      int>;
    // Main enqueue function.
    template <typename F, typename... Args, enabler<F &&, Args &&...> = 0>
    std::future<f_ret_type<F &&, Args &&...>> enqueue(F &&f, Args &&... args)
    {
        using ret_type = f_ret_type<F &&, Args &&...>;
        using p_task_type = std::packaged_task<ret_type()>;
        // NOTE: here we have a multi-stage construction of the task:
        // - std::bind() turns F into a nullary functor,
        // - std::packaged_task gives us the std::future machinery,
        // - std::function (in m_tasks) gives the uniform type interface via type erasure.
        auto task = std::make_shared<p_task_type>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<ret_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (unlikely(m_stop)) {
                // Enqueueing is not allowed if the queue is stopped.
                piranha_throw(std::runtime_error, "cannot enqueue task while the task queue is stopping");
            }
            m_tasks.push([task]() { (*task)(); });
        }
        // NOTE: notify_one is noexcept.
        m_cond.notify_one();
        return res;
    }
    // NOTE: we call this only from dtor, it is here in order to be able to test it.
    // So the exception handling in dtor will suffice, keep it in mind if things change.
    void stop()
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_stop) {
                // Already stopped.
                return;
            }
            m_stop = true;
        }
        // Notify the thread that queue has been stopped, wait for it
        // to consume the remaining tasks and exit.
        m_cond.notify_one();
        m_thread->join();
    }

    bool m_stop;
    std::condition_variable m_cond;
    std::mutex m_mutex;
    std::queue<std::function<void()>> m_tasks;
    std::unique_ptr<std::thread> m_thread;
};

// Type to represent thread queues: a vector of task queues paired with a set of thread ids.
using thread_queues_t = std::pair<std::vector<std::unique_ptr<task_queue>>, std::unordered_set<std::thread::id>>;

inline thread_queues_t get_initial_thread_queues()
{
    // NOTE: we used to have this print statement here, but it turns out that
    // in certain setups the cout object is not yet constructed at this point,
    // and a segfault is generated. I *think* it is possible to enforce the creation
    // of cout via construction of an init object:
    // http://en.cppreference.com/w/cpp/io/ios_base/Init
    // However, this is hardly essential. Let's leave this disabled for the moment.
    // std::cout << "Initializing the thread pool.\n";
    thread_queues_t retval;
    // Create the vector of queues.
    const unsigned candidate = runtime_info::get_hardware_concurrency(), hc = (candidate > 0u) ? candidate : 1u;
    retval.first.reserve(static_cast<decltype(retval.first.size())>(hc));
    for (unsigned i = 0u; i < hc; ++i) {
        // NOTE: thread binding is disabled on startup.
        retval.first.emplace_back(::new task_queue(i, false));
    }
    // Generate the set of thread IDs.
    for (const auto &ptr : retval.first) {
        auto p = retval.second.insert(ptr->m_thread->get_id());
        (void)p;
        piranha_assert(p.second);
    }
    return retval;
}

template <typename = void>
struct thread_pool_base {
    static thread_queues_t s_queues;
    static bool s_bind;
    static std::atomic_flag s_atf;
};

template <typename T>
thread_queues_t thread_pool_base<T>::s_queues = get_initial_thread_queues();

template <typename T>
std::atomic_flag thread_pool_base<T>::s_atf = ATOMIC_FLAG_INIT;

template <typename T>
bool thread_pool_base<T>::s_bind = false;

template <typename>
void thread_pool_shutdown();
}

/// Static thread pool.
/**
 * \note
 * The template parameter in this class is unused: its only purpose is to prevent the instantiation
 * of the class' methods if they are not explicitly used. Client code should always employ the
 * piranha::thread_pool alias.
 *
 * This class manages, via a set of static methods, a pool of threads created at program startup.
 * The number of threads created initially is equal to piranha::runtime_info::get_hardware_concurrency().
 * If the hardware concurrency cannot be determined, the size of the thread pool will be one.
 *
 * This class provides methods to enqueue arbitray tasks to the threads in the pool, query the size of the pool,
 * resize the pool and configure the thread binding policy. All methods, unless otherwise specified, are thread-safe,
 * and they provide the strong exception safety guarantee.
 */
// \todo work around MSVC bug in destruction of statically allocated threads (if needed once we support MSVC), as per:
// http://stackoverflow.com/questions/10915233/stdthreadjoin-hangs-if-called-after-main-exits-when-using-vs2012-rc
// detach() and wait as a workaround?
// \todo try to understand if we can suppress the future list class below in favour of STL-like algorithms.
template <typename T = void>
class thread_pool_ : private thread_pool_base<>
{
    friend void piranha::impl::thread_pool_shutdown<T>();
    using base = thread_pool_base<>;
    // Enabler for use_threads.
    template <typename Int>
    using use_threads_enabler
        = enable_if_t<disjunction<std::is_same<Int, integer>,
                                  conjunction<std::is_integral<Int>, std::is_unsigned<Int>>>::value,
                      int>;
    // The return type for enqueue().
    template <typename F, typename... Args>
    using enqueue_t = decltype(std::declval<task_queue &>().enqueue(std::declval<F>(), std::declval<Args>()...));

public:
    /// Enqueue task.
    /**
     * \note
     * This method is enabled only if:
     * - a nullary wrapper for <tt>F(args...)</tt> can be created via \p std::bind() (which requires \p F and
     *   \p Args to be copy/move constructible, and <tt>F(args...)</tt> to be a well-formed expression),
     * - the type returned by <tt>F(args...)</tt> satisfies piranha::is_returnable.
     *
     * This method will add a task to the <tt>n</tt>-th thread in the pool. The task is represented
     * by a callable \p F and its arguments \p args, which will be copied/moved via an \p std::bind() wrapper into an
     * execution queue consumed by the thread to which the task is assigned. The return value is an \p std::future
     * which can be used to retrieve the return value of (or the exception throw by) the callable.
     *
     * @param n index of the thread that will consume the task.
     * @param f callable object representing the task.
     * @param args arguments to \p f.
     *
     * @return an \p std::future that will store the result of <tt>f(args...)</tt>.
     *
     * @throws std::invalid_argument if the thread index is equal to or larger than the current pool size.
     * @throws std::runtime_error if a task is being enqueued while the task queue is stopping (e.g., during
     * program shutdown).
     * @throws unspecified any exception thrown by:
     * - \p std::bind() or the constructor of \p std::packaged_task or \p std::function,
     * - threading primitives,
     * - memory allocation errors.
     */
    template <typename F, typename... Args>
    static enqueue_t<F &&, Args &&...> enqueue(unsigned n, F &&f, Args &&... args)
    {
        detail::atomic_lock_guard lock(s_atf);
        if (n >= s_queues.first.size()) {
            piranha_throw(std::invalid_argument, "the thread index " + std::to_string(n)
                                                     + " is out of range, the thread pool contains only "
                                                     + std::to_string(s_queues.first.size()) + " threads");
        }
        return base::s_queues.first[static_cast<decltype(base::s_queues.first.size())>(n)]->enqueue(
            std::forward<F>(f), std::forward<Args>(args)...);
    }
    /// Size
    /**
     * @return the number of threads in the pool.
     */
    static unsigned size()
    {
        detail::atomic_lock_guard lock(s_atf);
        return static_cast<unsigned>(base::s_queues.first.size());
    }

private:
    // Helper function to create 'new_size' new queues with thread binding set to 'bind'.
    static thread_queues_t create_new_queues(unsigned new_size, bool bind)
    {
        thread_queues_t new_queues;
        // Create the task queues.
        new_queues.first.reserve(static_cast<decltype(new_queues.first.size())>(new_size));
        for (auto i = 0u; i < new_size; ++i) {
            new_queues.first.emplace_back(::new task_queue(i, bind));
        }
        // Fill in the thread ids set.
        for (const auto &ptr : new_queues.first) {
            auto p = new_queues.second.insert(ptr->m_thread->get_id());
            (void)p;
            piranha_assert(p.second);
        }
        return new_queues;
    }
    // Shutdown. This can be used to stop the threads at program shutdown.
    static void shutdown()
    {
        thread_queues_t new_queues;
        detail::atomic_lock_guard lock(s_atf);
        new_queues.swap(base::s_queues);
    }

public:
    /// Change the number of threads.
    /**
     * This method will resize the internal pool to contain \p new_size threads. The method will first wait for
     * the threads to consume all the pending tasks (while forbidding the addition of new tasks), and it will then
     * create a new pool of size \p new_size.
     *
     * @param new_size the new size of the pool.
     *
     * @throws std::invalid_argument if \p new_size is zero.
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - memory allocation errors.
     */
    static void resize(unsigned new_size)
    {
        if (unlikely(new_size == 0u)) {
            piranha_throw(std::invalid_argument, "cannot resize the thread pool to zero");
        }
        // NOTE: need to lock here as we are reading the s_bind member.
        detail::atomic_lock_guard lock(s_atf);
        auto new_queues = create_new_queues(new_size, base::s_bind);
        // NOTE: here the allocator is not swapped, as std::allocator won't propagate on swap.
        // Besides, all instances of std::allocator are equal, so the operation is well-defined.
        // http://en.cppreference.com/w/cpp/container/vector/swap
        // This holds for both std::vector and std::unordered_set.
        // If an exception gets actually thrown, no big deal.
        // NOTE: the dtor of the queues is effectively noexcept, as the program will just abort in case of errors
        // in the dtor.
        new_queues.swap(base::s_queues);
    }
    /// Set the thread binding policy.
    /**
     * If \p flag is \p true, this method will bind each thread in the pool to a different processor/core via
     * piranha::bind_to_proc(). If \p flag is \p false, then this method will unbind the threads in the pool from any
     * processor/core to which they might be bound.
     *
     * The threads created at program startup are not bound to any specific processor/core. Any error raised by
     * piranha::bind_to_proc() (e.g., because the number of threads in the pool is larger than the number of logical
     * cores or because the thread binding functionality is not available on the platform) is silently ignored.
     *
     * @param flag the desired thread binding policy.
     *
     * @throws unspecified any exception thrown by:
     * - threading primitives,
     * - memory allocation errors.
     */
    static void set_binding(bool flag)
    {
        detail::atomic_lock_guard lock(s_atf);
        if (flag == base::s_bind) {
            // Don't do anything if we are not changing the binding policy.
            return;
        }
        auto new_queues = create_new_queues(static_cast<unsigned>(base::s_queues.first.size()), flag);
        new_queues.swap(base::s_queues);
        base::s_bind = flag;
    }
    /// Get the thread binding policy.
    /**
     * This method returns the flag set by set_binding(). The threads created on program startup are not bound to
     * specific processors/cores.
     *
     * @return a boolean flag representing the active thread binding policy.
     */
    static bool get_binding()
    {
        detail::atomic_lock_guard lock(s_atf);
        return base::s_bind;
    }
    /// Compute number of threads to use.
    /**
     * \note
     * This function is enabled only if \p Int is an unsigned integer type or piranha::integer.
     *
     * This function computes the suggested number of threads to use, given an amount of total \p work_size units of
     * work and a minimum amount of work units per thread \p min_work_per_thread.
     *
     * The returned value will always be 1 if the calling thread belongs to the thread pool; otherwise, a number of
     * threads such that each thread has at least \p min_work_per_thread units of work to consume will be returned.
     * In any case, the return value is always greater than zero.
     *
     * @param work_size total number of work units.
     * @param min_work_per_thread minimum number of work units to be consumed by a thread in the pool.
     *
     * @return the suggested number of threads to be used, always greater than zero.
     *
     * @throws std::invalid_argument if \p work_size or \p min_work_per_thread are not strictly positive.
     * @throws unspecified any exception thrown by \p boost::lexical_cast().
     */
    template <typename Int, use_threads_enabler<Int> = 0>
    static unsigned use_threads(const Int &work_size, const Int &min_work_per_thread)
    {
        // Check input params.
        if (unlikely(work_size <= Int(0))) {
            piranha_throw(std::invalid_argument, "invalid value of " + boost::lexical_cast<std::string>(work_size)
                                                     + " for work size (it must be strictly positive)");
        }
        if (unlikely(min_work_per_thread <= Int(0))) {
            piranha_throw(std::invalid_argument, "invalid value of "
                                                     + boost::lexical_cast<std::string>(min_work_per_thread)
                                                     + " for minimum work per thread (it must be strictly positive)");
        }
        detail::atomic_lock_guard lock(s_atf);
        // Don't use threads if the calling thread belongs to the pool.
        if (base::s_queues.second.find(std::this_thread::get_id()) != base::s_queues.second.end()) {
            return 1u;
        }
        const auto n_threads = static_cast<unsigned>(base::s_queues.first.size());
        piranha_assert(n_threads);
        if (work_size / n_threads >= min_work_per_thread) {
            // Enough work per thread, use them all.
            return n_threads;
        }
        // Return a number of threads such that each thread consumes at least min_work_per_thread.
        // Never return 0.
        return static_cast<unsigned>(std::max(Int(1), static_cast<Int>(work_size / min_work_per_thread)));
    }
};

/// Alias for piranha::thread_pool_.
/**
 * This is the alias through which the methods in piranha::thread_pool_ should be called.
 */
using thread_pool = thread_pool_<>;

inline namespace impl
{

template <typename>
inline void thread_pool_shutdown()
{
    thread_pool::shutdown();
}
}

/// Class to store a list of futures.
/**
 * This class is a minimal thin wrapper around an \p std::list of \p std::future<T> objects.
 * The class provides convenience methods to interact with the set of futures in an exception-safe manner.
 */
// NOTE: we could provide method to retrieve future values from get_all() using a vector (in case the future type
// is not void or a reference, in which case the get_all() method stays as it is).
template <typename T>
class future_list
{
    // Wait on a valid future, or abort.
    static void wait_or_abort(const std::future<T> &fut)
    {
        piranha_assert(fut.valid());
        try {
            fut.wait();
        } catch (...) {
            // NOTE: logging candidate, with info from exception.
            std::abort();
        }
    }

public:
    /// Defaulted default constructor.
    /**
     * This constructor will initialise an empty list of futures.
     */
    future_list() = default;
    /// Deleted copy constructor.
    future_list(const future_list &) = delete;
    /// Deleted move constructor.
    future_list(future_list &&) = delete;
    /// Deleted copy assignment.
    future_list &operator=(const future_list &) = delete;
    /// Deleted move assignment.
    future_list &operator=(future_list &&) = delete;
    /// Destructor
    /**
     * Will call wait_all().
     */
    ~future_list()
    {
        wait_all();
    }
    /// Move-insert a future.
    /**
     * Will move-insert the input future \p f into the internal container.
     * If the insertion fails due to memory allocation errors and \p f is a valid
     * future, then the method will wait on \p f before throwing the exception.
     *
     * @param f std::future to be move-inserted.
     *
     * @throws unspecified any exception thrown by memory allocation errors.
     */
    void push_back(std::future<T> &&f)
    {
        // Push back empty future.
        try {
            m_list.emplace_back();
        } catch (...) {
            // If we get some error here, we want to make sure we wait on the future
            // before escaping out.
            // NOTE: calling wait() on an invalid future is UB.
            if (f.valid()) {
                wait_or_abort(f);
            }
            throw;
        }
        // This cannot throw.
        m_list.back() = std::move(f);
    }
    /// Wait on all the futures.
    /**
     * This method will call <tt>wait()</tt> on all the valid futures stored within the object.
     */
    void wait_all()
    {
        for (auto &f : m_list) {
            if (f.valid()) {
                wait_or_abort(f);
            }
        }
    }
    /// Get all the futures.
    /**
     * This method will call <tt>get()</tt> on all the valid futures stored within the object.
     * The return values resulting from the calls to <tt>get()</tt> will be ignored.
     *
     * @throws unspecified an exception stored by a future.
     */
    void get_all()
    {
        for (auto &f : m_list) {
            // NOTE: std::future's valid() method is noexcept.
            if (f.valid()) {
                (void)f.get();
            }
        }
    }

private:
    std::list<std::future<T>> m_list;
};
}

#endif
