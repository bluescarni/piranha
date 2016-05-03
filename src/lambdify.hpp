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

// Enabler for lambdify().
template <typename T, typename U>
using lambdify_enabler = typename std::enable_if<is_evaluable<T,U>::value &&
    is_mappable<U>::value,int>::type;

}

/// Function-like evaluation of symbolic objects.
/**
 * This class exposes a function-like interface for the evaluation of instances of type \p T in which
 * symbolic quantities are subtituted with objects of type \p U.
 *
 * In essence, thic class acts as a small wrapper for piranha::math::evaluate() which replaces the interface
 * based on \p std::unordered_map with an interface based on vectors.
 *
 * The convenience function piranha::lambdify() can be used to easily construct objects of this class.
 */
// TODO type requirements.
template <typename T, typename U>
class lambdified
{
        PIRANHA_TT_CHECK(is_evaluable,T,U);
        PIRANHA_TT_CHECK(is_mappable,U);
        using eval_type = decltype(math::evaluate(std::declval<const T &>(),
            std::declval<const std::unordered_map<std::string,U> &>()));
    public:
        /// Constructor.
        /**
         * This constructor will create an internal copy of \p x, the object that will be used for evaluation.
         * The vector of string \p names indicates in which order the symbols in \p x are to be considered
         * in the call to operator()(). That is, the values in the vector passed to operator()() are associated
         * internally to the symbols in \p names at the corresponding positions.
         *
         * @param[in] x the object that will be evaluated by calls to operator()().
         * @param[in] names the list of symbols to which the values passed to operator()() will be mapped.
         *
         * @throws std::invalid_argument if \p names contains duplicates.
         * @throws unspecified any exception thrown by:
         * - memory errors in standard containers,
         * - the public interface of std::unordered_map,
         * - the construction of objects of type \p U.
         */
        explicit lambdified(const T &x, const std::vector<std::string> &names):m_x(x)
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
        /// Evaluation.
        /**
         * The call operator will first associate the elements of \p values to the vector of names used to construct \p this,
         * and it will then call piranha::math::evaluate() on the stored internal copy of the object of type \p T used
         * during construction.
         *
         * @param[in] values the values that will be used for evaluation.
         * @return [description]
         */
        eval_type operator()(const std::vector<U> &values)
        {
            if (unlikely(values.size() != m_eval_dict.size())) {
                piranha_throw(std::invalid_argument,"the size of the vector of evaluation values does not "
                    "match the size of the symbol list used during construction");
            }
            for (decltype(values.size()) i = 0u; i < values.size(); ++i) {
                *m_ptrs[static_cast<decltype(m_ptrs.size())>(i)] = values[i];
            }
            return math::evaluate(m_x,m_eval_dict);
        }
    private:
        const T m_x;
        std::unordered_map<std::string,U> m_eval_dict;
        std::vector<U *> m_ptrs;
};

template <typename U, typename T, detail::lambdify_enabler<T,U> = 0>
inline lambdified<T,U> lambdify(const T &x, const std::vector<std::string> &names)
{
    return lambdified<T,U>(x,names);
}

template <typename T, typename U>
class has_lambdify: detail::sfinae_types
{
        using Td = typename std::decay<T>::type;
        using Ud = typename std::decay<U>::type;
        template <typename T1, typename U1>
        static auto test(const T1 &x, const U1 &) -> decltype(lambdify<U1>(x,{}),void(),yes());
        static no test(...);
        static const bool implementation_defined = std::is_same<yes,decltype(test(std::declval<const Td &>(),
            std::declval<const Ud &>()))>::value;
    public:
        static const bool value = implementation_defined;
};

}

#endif
