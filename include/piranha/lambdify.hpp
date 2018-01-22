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

#ifndef PIRANHA_LAMBDIFY_HPP
#define PIRANHA_LAMBDIFY_HPP

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <piranha/detail/init.hpp>
#include <piranha/detail/sfinae_types.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/math.hpp>
#include <piranha/symbol_utils.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace detail
{

// Requirements for the lambdified template parameters (after decay).
template <typename T, typename U>
using math_lambdified_reqs = std::integral_constant<
    bool, conjunction<is_evaluable<T, U>, std::is_copy_constructible<T>, std::is_move_constructible<T>>::value>;
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
 * - \p T must be copy and move constructible.
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
    static_assert(std::is_same<T, typename std::decay<T>::type>::value, "Invalid type.");
    static_assert(std::is_same<U, typename std::decay<U>::type>::value, "Invalid type.");
    static_assert(detail::math_lambdified_reqs<T, U>::value, "Invalid types.");
    // Constructor implementation.
    void construct()
    {
        // Check if there are duplicates.
        auto names_copy(m_names);
        std::sort(names_copy.begin(), names_copy.end());
        if (std::unique(names_copy.begin(), names_copy.end()) != names_copy.end()) {
            piranha_throw(std::invalid_argument, "the list of evaluation symbols contains duplicates");
        }
        // Fill in the eval dict and the vector of pointers, and make sure
        // that m_extra_map does not contain anything that is already in m_names.
        for (const auto &s : m_names) {
            if (m_extra_map.find(s) != m_extra_map.end()) {
                piranha_throw(std::invalid_argument,
                              "the extra symbols map contains symbol '" + s
                                  + "', which is already in the symbol list used for the construction "
                                    "of the lambdified object");
            }
            const auto ret = m_eval_dict.emplace(std::make_pair(s, U{}));
            piranha_assert(ret.second);
            m_ptrs.push_back(std::addressof(ret.first->second));
        }
        // Fill in the extra symbols.
        for (const auto &p : m_extra_map) {
            const auto ret = m_eval_dict.emplace(std::make_pair(p.first, U{}));
            piranha_assert(ret.second);
            m_ptrs.push_back(std::addressof(ret.first->second));
        }
    }
    // Reconstruct the vector of pointers following copy or move construction.
    void reconstruct_ptrs()
    {
        // m_ptrs must be empty because we assume this method is called only as part of
        // the copy/move ctors.
        piranha_assert(m_ptrs.empty());
        piranha_assert(m_eval_dict.size() >= m_extra_map.size()
                       && m_names.size() == m_eval_dict.size() - m_extra_map.size());
        // Take care first of the positional argument.
        for (const auto &s : m_names) {
            piranha_assert(m_eval_dict.count(s) == 1u);
            m_ptrs.push_back(std::addressof(m_eval_dict.find(s)->second));
        }
        // The extra symbols.
        for (const auto &p : m_extra_map) {
            piranha_assert(m_eval_dict.count(p.first) == 1u);
            m_ptrs.push_back(std::addressof(m_eval_dict.find(p.first)->second));
        }
        // Make sure the sizes are consistent.
        piranha_assert(m_ptrs.size() == static_cast<decltype(m_ptrs.size())>(m_names.size()) + m_extra_map.size());
    }

public:
    /// Evaluation type.
    /**
     * This is the type resulting from evaluating objects of type \p T with objects of type \p U
     * via piranha::math::evaluate().
     */
    using eval_type = decltype(math::evaluate(std::declval<const T &>(), std::declval<const symbol_fmap<U> &>()));
    /// The map type for the custom evaluation of symbols.
    /**
     * See the constructor documentation for an explanation of how this type is used.
     */
    using extra_map_type = std::unordered_map<std::string, std::function<U(const std::vector<U> &)>>;
    /// Constructor.
    /**
     * This constructor will create an internal copy of \p x, the object that will be evaluated
     * by successive calls to operator()().
     * The vector of string \p names establishes the correspondence between symbols and the values
     * with which the symbols will be replaced when operator()() is called.
     * That is, the values in the vector passed to operator()() are associated
     * to the symbols in \p names at the corresponding positions.
     *
     * The optional argument \p extra_map can be used to specify how to evaluate specific symbols.
     * That is, \p extra_map associates symbol names to functions of signature
     * @code
     * U (const std::vector<U> &)
     * @endcode
     * When operator()() is called with a vector of values \p v, for each symbol \p s in \p extra_map
     * the associated function is called with \p v as an argument, and the return value is used as evaluation
     * value for \p s in the subsequent call to math::evaluate(). \p extra_map must not contain symbol names appearing
     * in \p names.
     *
     * @param x the object that will be evaluated by operator()().
     * @param names the list of symbols to which the values passed to operator()() will be mapped.
     * @param extra_map the custom symbol evaluation map.
     *
     * @throws std::invalid_argument if \p names contains duplicates or if \p extra_map contains symbol names
     * already present in \p names.
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - the public interface of std::unordered_map,
     * - the copy constructor of \p T,
     * - the construction of objects of type \p U.
     */
    explicit lambdified(const T &x, const std::vector<std::string> &names, extra_map_type extra_map = {})
        : m_x(x), m_names(names), m_extra_map(extra_map)
    {
        construct();
    }
    /// Constructor (move overload).
    /**
     * This constructor is equivalent to the other constructor, the only difference being that \p x
     * is used to move-construct (instead of copy-construct) the internal instance of \p T.
     *
     * @param x the object that will be evaluated by operator()().
     * @param names the list of symbols to which the values passed to operator()() will be mapped.
     * @param extra_map the custom symbol evaluation map.
     *
     * @throws std::invalid_argument if \p names contains duplicates or if \p extra_map contains symbol names
     * already present in \p names.
     * @throws unspecified any exception thrown by:
     * - memory errors in standard containers,
     * - the public interface of std::unordered_map,
     * - the move constructor of \p T,
     * - the construction of objects of type \p U.
     */
    explicit lambdified(T &&x, const std::vector<std::string> &names, extra_map_type extra_map = {})
        : m_x(std::move(x)), m_names(names), m_extra_map(extra_map)
    {
        construct();
    }
    /// Copy constructor.
    /**
     * @param other copy argument.
     *
     * @throws unspecified any exception thrown by the copy constructor of the internal members.
     */
    lambdified(const lambdified &other)
        : m_x(other.m_x), m_names(other.m_names), m_eval_dict(other.m_eval_dict), m_extra_map(other.m_extra_map)
    {
        reconstruct_ptrs();
    }
    /// Move constructor.
    /**
     * @param other move argument.
     *
     * @throws unspecified any exception thrown by the move constructor of the internal members.
     */
    lambdified(lambdified &&other)
        : m_x(std::move(other.m_x)), m_names(std::move(other.m_names)), m_eval_dict(std::move(other.m_eval_dict)),
          m_extra_map(std::move(other.m_extra_map))
    {
        // NOTE: it looks like we cannot be sure the moved-in pointers are still valid.
        // Let's just make sure.
        reconstruct_ptrs();
    }

private:
    lambdified &operator=(const lambdified &) = delete;
    lambdified &operator=(lambdified &&) = delete;

public:
    /// Evaluation.
    /**
     * The call operator will first associate the elements of \p values to the vector of names used to construct \p
     * this,
     * and it will then call piranha::math::evaluate() on the stored internal instance of the object of type \p T used
     * during construction.
     *
     * If a non-empty \p extra_map parameter was used during construction, the symbols in it are evaluated according
     * to the mapped functions before being passed down in the evaluation dictionary to piranha::math::evaluate().
     *
     * Note that this function needs to modify the internal state of the object, and thus it is not const and it is
     * not thread-safe.
     *
     * @param values the values that will be used for evaluation.
     *
     * @return the output of piranha::math::evaluate() called on the instance of type \p T stored internally.
     *
     * @throws std::invalid_argument if the size of \p values is not equal to the size of the vector of names
     * used during construction.
     * @throws unspecified any exception raised by:
     * - the copy-assignment operator of \p U,
     * - math::evaluate(),
     * - the call operator of the mapped functions in the \p extra_map parameter used during construction.
     */
    eval_type operator()(const std::vector<U> &values)
    {
        if (unlikely(values.size() != m_eval_dict.size() - m_extra_map.size())) {
            piranha_throw(std::invalid_argument, "the size of the vector of evaluation values does not "
                                                 "match the size of the symbol list used during construction");
        }
        piranha_assert(values.size() == m_names.size());
        decltype(values.size()) i = 0u;
        for (; i < values.size(); ++i) {
            auto ptr = m_ptrs[static_cast<decltype(m_ptrs.size())>(i)];
            piranha_assert(
                ptr == std::addressof(m_eval_dict.find(m_names[static_cast<decltype(m_names.size())>(i)])->second));
            *ptr = values[i];
        }
        // NOTE: here it is crucial that the iteration order on m_extra_map is the same
        // as it was during the construction of the m_ptrs vector, otherwise we are associating
        // values to the wrong symbols. Luckily, it seems like iterating on a const unordered_map
        // multiple times yields the same order:
        // http://stackoverflow.com/questions/18301302/is-forauto-i-unordered-map-guaranteed-to-have-the-same-order-every-time
        // https://groups.google.com/a/isocpp.org/forum/#!topic/std-discussion/kHYFUhsauhU
        for (const auto &p : m_extra_map) {
            piranha_assert(i < m_ptrs.size());
            auto ptr = m_ptrs[static_cast<decltype(m_ptrs.size())>(i)];
            piranha_assert(ptr == std::addressof(m_eval_dict.find(p.first)->second));
            *ptr = p.second(values);
            ++i;
        }
        // NOTE: of course, this will have to be fixed in the rewrite.
        return math::evaluate(m_x, symbol_fmap<U>{m_eval_dict.begin(), m_eval_dict.end()});
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
    /// Get names of the symbols in the extra map.
    /**
     * @return a vector containing the names of the symbols in the \p extra_map used during construction.
     *
     * @throws unspecified any exception thrown by memory errors in standard containers.
     */
    std::vector<std::string> get_extra_names() const
    {
        std::vector<std::string> retval;
        std::transform(m_extra_map.begin(), m_extra_map.end(), std::back_inserter(retval),
                       [](const typename extra_map_type::value_type &p) { return p.first; });
        return retval;
    }

private:
    T m_x;
    std::vector<std::string> m_names;
    std::unordered_map<std::string, U> m_eval_dict;
    std::vector<U *> m_ptrs;
    extra_map_type m_extra_map;
};
}

namespace detail
{

// Enabler for lambdify().
template <typename T, typename U>
using math_lambdify_type =
    typename std::enable_if<math_lambdified_reqs<typename std::decay<T>::type, typename std::decay<U>::type>::value,
                            math::lambdified<typename std::decay<T>::type, typename std::decay<U>::type>>::type;

// Shortcut for the extra symbols map type.
template <typename T, typename U>
using math_lambdify_extra_map_type = typename math_lambdify_type<T, U>::extra_map_type;
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
 * to evaluate \p x with a function-like interface. The parameters of this function are passed
 * to the constructor of the returned piranha::math::lambdified object. For example:
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
 * The optional parameter \p extra_map (of type piranha::math::lambdified::extra_map_type) is a map specifying how
 * specific symbols should be evaluated. It is most useful when symbols have an implicit dependency on other symbols.
 * For instance, suppose that the symbol \f$z\f$ is implicitly dependent on the symbols \f$x\f$ and \f$y\f$ via
 * \f$z\left(x,y\right) = \sqrt{x+y}\f$. Then in order to evaluate \f$x + y + z\f$ we can write:
 * @code
 * polynomial<integer,k_monomial> x{"x"}, y{"y"}, z{"z"};
 * auto l = lambdify<double>(x+y+z,{"x","y"},{{"z",[](const std::vector<double> &v) {return std::sqrt(v[0]+v[1]);}}});
 * std::cout << l({1.,2.}) << '\n' // This will print 1.+2.+sqrt(1.+2.) = 4.7320508076...
 * @endcode
 * See the constructor of piranha::math::lambdified for more details on the \p extra_map argument.
 *
 * The decay types of \p T and \p U are used as template parameters for the piranha::math::lambdified return type.
 *
 * @param x object that will be evaluated.
 * @param names names of the symbols that will be used for evaluation.
 * @param extra_map map of type piranha::math::lambdified::extra_map_type for custom symbol evaluation.
 *
 * @return an instance of piranha::math::lambdified that can be used to evaluate \p x.
 *
 * @throws unspecified any exception thrown by the constructor of piranha::math::lambdified.
 */
template <typename U, typename T>
inline detail::math_lambdify_type<T, U> lambdify(T &&x, const std::vector<std::string> &names,
                                                 detail::math_lambdify_extra_map_type<T, U> extra_map = {})
{
    return lambdified<typename std::decay<T>::type, typename std::decay<U>::type>(std::forward<T>(x), names, extra_map);
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
class has_lambdify : detail::sfinae_types
{
    using Td = typename std::decay<T>::type;
    using Ud = typename std::decay<U>::type;
    template <typename T1, typename U1>
    static auto test(const T1 &x, const U1 &) -> decltype(math::lambdify<U1>(x, {}), void(), yes());
    static no test(...);
    static const bool implementation_defined
        = std::is_same<yes, decltype(test(std::declval<const Td &>(), std::declval<const Ud &>()))>::value;

public:
    /// Value of the type trait.
    static const bool value = implementation_defined;
};

template <typename T, typename U>
const bool has_lambdify<T, U>::value;
}

#endif
