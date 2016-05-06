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

#ifndef PIRANHA_LAMBDIFY_HPP
#define PIRANHA_LAMBDIFY_HPP

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "detail/sfinae_types.hpp"
#include "exceptions.hpp"
#include "math.hpp"
#include "type_traits.hpp"

namespace piranha
{

namespace detail
{

// Requirements for the lambdified template parameters (after decay).
template <typename T, typename U>
using math_lambdified_reqs = std::integral_constant<bool,
        is_evaluable<T,U>::value && is_mappable<U>::value &&
        std::is_copy_constructible<T>::value && std::is_move_constructible<T>::value>;

}

namespace math
{

/// Functor interface for piranha::math::evaluate().
/**
 * This class exposes a function-like interface for the evaluation of instances of type \p T with objects of type \p U.
 * The class acts as a small wrapper for piranha::math::evaluate() which replaces the interface
 * based on \p std::unordered_map with an interface based on vectors and positional arguments.
 *
 * The convenience function piranha::math::lambdify() can be used to easily construct objects of this class.
 *
 * ## Type requirements ##
 *
 * - \p T and \p U must be the same as their decay types,
 * - \p T must be evaluable with objects of type \p U,
 * - \p T must be copy and move constructible,
 * - \p U must satisfy piranha::is_mappable.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * After a move operation, an object of this class is destructible.
 */
template <typename T, typename U>
class lambdified
{
        static_assert(std::is_same<T,typename std::decay<T>::type>::value,"Invalid type.");
        static_assert(std::is_same<U,typename std::decay<U>::type>::value,"Invalid type.");
        static_assert(detail::math_lambdified_reqs<T,U>::value,"Invalid types.");
        using eval_type = decltype(math::evaluate(std::declval<const T &>(),
            std::declval<const std::unordered_map<std::string,U> &>()));
        // Constructor implementation.
        void construct(const std::vector<std::string> &names)
        {
            // Check if there are duplicates.
            auto names_copy(names);
            std::sort(names_copy.begin(),names_copy.end());
            if (std::unique(names_copy.begin(),names_copy.end()) != names_copy.end()) {
                piranha_throw(std::invalid_argument,"the list of evaluation symbols contains duplicates");
            }
            // Fill in the eval dict and the vector of pointers.
            for (const auto &s: names) {
                const auto ret = m_eval_dict.emplace(std::make_pair(s,U{}));
                piranha_assert(ret.second);
                m_ptrs.push_back(std::addressof(ret.first->second));
            }
        }
        // Reconstruct the vector of pointers after a copy or move operation.
        void reconstruct_ptrs()
        {
            piranha_assert(m_ptrs.empty());
            piranha_assert(m_names.size() == m_eval_dict.size());
            for (const auto &s: m_names) {
                piranha_assert(m_eval_dict.count(s) == 1u);
                m_ptrs.push_back(std::addressof(m_eval_dict.find(s)->second));
            }
        }
    public:
        /// Constructor.
        /**
         * This constructor will create an internal copy of \p x, the object that will be evaluated
         * by successive calls to operator()().
         * The vector of string \p names establishes the correspondence between symbols and the values
         * with which the symbols will be replaced when operator()() is called.
         * That is, the values in the vector passed to operator()() are associated
         * to the symbols in \p names at the corresponding positions.
         *
         * @param[in] x the object that will be evaluated by operator()().
         * @param[in] names the list of symbols to which the values passed to operator()() will be mapped.
         *
         * @throws std::invalid_argument if \p names contains duplicates.
         * @throws unspecified any exception thrown by:
         * - memory errors in standard containers,
         * - the public interface of std::unordered_map,
         * - the copy constructor of \p T,
         * - the construction of objects of type \p U.
         */
        explicit lambdified(const T &x, const std::vector<std::string> &names):m_x(x),m_names(names)
        {
            construct(names);
        }
        /// Constructor (move overload).
        /**
         * This constructor is equivalent to the other constructor, the only difference being that \p x
         * is used to move-construct (instead of copy-construct) the internal instance of \p T.
         *
         * @param[in] x the object that will be evaluated by operator()().
         * @param[in] names the list of symbols to which the values passed to operator()() will be mapped.
         *
         * @throws std::invalid_argument if \p names contains duplicates.
         * @throws unspecified any exception thrown by:
         * - memory errors in standard containers,
         * - the public interface of std::unordered_map,
         * - the move constructor of \p T,
         * - the construction of objects of type \p U.
         */
        explicit lambdified(T &&x, const std::vector<std::string> &names):m_x(std::move(x)),m_names(names)
        {
            construct(names);
        }
        /// Copy constructor.
        /**
         * @param[in] other copy argument.
         *
         * @throws unspecified any exception thrown by the copy constructor of the internal members.
         */
        lambdified(const lambdified &other):m_x(other.m_x),m_names(other.m_names),m_eval_dict(other.m_eval_dict)
        {
            reconstruct_ptrs();
        }
        /// Move constructor.
        /**
         * @param[in] other move argument.
         *
         * @throws unspecified any exception thrown by the move constructor of the internal members.
         */
        lambdified(lambdified &&other):m_x(std::move(other.m_x)),m_names(std::move(other.m_names)),
            m_eval_dict(std::move(other.m_eval_dict))
        {
            // NOTE: it looks like we cannot be sure the moved-in pointers are still valid.
            // Let's just make sure.
            reconstruct_ptrs();
        }
        /// Deleted copy assignment operator.
        lambdified &operator=(const lambdified &) = delete;
        /// Deleted move assignment operator.
        lambdified &operator=(lambdified &&) = delete;
        /// Evaluation.
        /**
         * The call operator will first associate the elements of \p values to the vector of names used to construct \p this,
         * and it will then call piranha::math::evaluate() on the stored internal instance of the object of type \p T used
         * during construction.
         *
         * Note that this function needs to modify the internal state of the object, and thus it is not const and it is
         * not thread-safe.
         *
         * @param[in] values the values that will be used for evaluation.
         *
         * @return the output of piranha::math::evaluate() called on the instance of type \p T stored internally.
         *
         * @throws std::invalid_argument if the size of \p values is not equal to the size of the vector of names
         * used during construction.
         * @throws unspecified any exception raised by:
         * - the copy-assignment operator of \p U,
         * - math::evaluate().
         */
        eval_type operator()(const std::vector<U> &values)
        {
            if (unlikely(values.size() != m_eval_dict.size())) {
                piranha_throw(std::invalid_argument,"the size of the vector of evaluation values does not "
                    "match the size of the symbol list used during construction");
            }
            piranha_assert(values.size() == m_names.size());
            for (decltype(values.size()) i = 0u; i < values.size(); ++i) {
                auto ptr = m_ptrs[static_cast<decltype(m_ptrs.size())>(i)];
                piranha_assert(ptr == std::addressof(m_eval_dict.find(
                    m_names[static_cast<decltype(m_names.size())>(i)])->second));
                *ptr = values[i];
            }
            return math::evaluate(m_x,m_eval_dict);
        }
        /// Get evaluation object.
        /**
         * @return a const reference to the internal copy of the object of type \p T created
         * upon construction.
         */
        const T &get_evaluable() const
        {
            return m_x;
        }
        /// Get evaluation names.
        /**
         * @return a const reference to the vector of symbol names used as second construction argument.
         */
        const std::vector<std::string> &get_names() const
        {
            return m_names;
        }
    private:
        const T m_x;
        const std::vector<std::string> m_names;
        std::unordered_map<std::string,U> m_eval_dict;
        std::vector<U *> m_ptrs;
};

}

namespace detail
{

// Enabler for lambdify().
template <typename T, typename U>
using math_lambdify_type = typename std::enable_if<
        math_lambdified_reqs<typename std::decay<T>::type,typename std::decay<U>::type>::value,
        math::lambdified<typename std::decay<T>::type, typename std::decay<U>::type>>::type;

}

namespace math
{

/// Create a functor interface for piranha::math::evaluate().
/**
 * \note
 * This function is enabled only if the decay types of \p T and \p U can be used as template parameters in
 * piranha::math::lambdified.
 *
 * This utility function will create an object of type piranha::math::lambdified that can be used
 * to evaluate \p x with a function-like interface. For example:
 * @code
 * polynomial<integer,k_monomial> x{"x"}, y{"y"}, z{"z"};
 * auto l = lambdify<double>(x-2*y+3*z,{"z","y","x"});
 * @endcode
 * The object \p l can then be used to evaluate <tt>x-2*y+3*z</tt> in the following way:
 * @code
 * l({1.,2.,3.});
 * @endcode
 * That is, <tt>x-2*y+3*z</tt> is evaluated with <tt>x=3.</tt>, <tt>y=2.</tt> and <tt>z=1.</tt>.
 *
 * The decay types of \p T and \p U are used as template parameters for the piranha::math::lambdified return type.
 *
 * @param[in] x object that will be evaluated.
 * @param[in] names names of the symbols that will be used for evaluation.
 *
 * @return an instance of piranha::math::lambdified that can be used to evaluate \p x.
 *
 * @throws unspecified any exception thrown by the constructor of piranha::math::lambdified.
 */
template <typename U, typename T>
inline detail::math_lambdify_type<T,U> lambdify(T &&x, const std::vector<std::string> &names)
{
    return lambdified<typename std::decay<T>::type, typename std::decay<U>::type>(
        std::forward<T>(x),names);
}

}

/// Detect the presence of piranha::math::lambdify().
/**
 * This type trait will be \p true if piranha::math::lambdify() can be called with a first argument of type
 * \p T, and evaluation type \p U.
 *
 * The decay types of \p T and \p U are considered by this type trait.
 */
template <typename T, typename U>
class has_lambdify: detail::sfinae_types
{
        using Td = typename std::decay<T>::type;
        using Ud = typename std::decay<U>::type;
        template <typename T1, typename U1>
        static auto test(const T1 &x, const U1 &) -> decltype(math::lambdify<U1>(x,{}),void(),yes());
        static no test(...);
        static const bool implementation_defined = std::is_same<yes,decltype(test(std::declval<const Td &>(),
            std::declval<const Ud &>()))>::value;
    public:
        /// Value of the type trait.
        static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_lambdify<T,U>::value;

}

#endif
