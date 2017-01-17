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

#ifndef PYRANHA_EXPOSE_UTILS_HPP
#define PYRANHA_EXPOSE_UTILS_HPP

#include "python_includes.hpp"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/import.hpp>
#include <boost/python/init.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/tuple.hpp>
#include <cstddef>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include <piranha/detail/demangle.hpp>
#include <piranha/detail/sfinae_types.hpp>
#include <piranha/detail/type_in_tuple.hpp>
#include <piranha/invert.hpp>
#include <piranha/lambdify.hpp>
#include <piranha/math.hpp>
#include <piranha/mp_integer.hpp>
#include <piranha/mp_rational.hpp>
#include <piranha/pow.hpp>
#include <piranha/power_series.hpp>
#include <piranha/real.hpp>
#include <piranha/s11n.hpp>
#include <piranha/series.hpp>
#include <piranha/type_traits.hpp>

#include "type_system.hpp"
#include "utils.hpp"

namespace pyranha
{

namespace bp = boost::python;

// Generic pickle support via Boost serialization.
template <typename Series>
struct generic_pickle_suite : bp::pickle_suite {
    static bp::tuple getinitargs(const Series &)
    {
        return bp::make_tuple();
    }
    static bp::tuple getstate(const Series &s)
    {
        std::stringstream ss;
        {
            // NOTE: use text archive by default, as it's the safest. Maybe in the future
            // we can make the pickle serialization backend selectable by the user.
            boost::archive::text_oarchive oa(ss);
            oa << s;
        }
        return bp::make_tuple(ss.str());
    }
    static void setstate(Series &s, bp::tuple state)
    {
        if (bp::len(state) != 1) {
            ::PyErr_SetString(PyExc_ValueError, "the 'state' tuple must have exactly one element");
            bp::throw_error_already_set();
        }
        std::string st = bp::extract<std::string>(state[0]);
        std::stringstream ss;
        ss.str(st);
        boost::archive::text_iarchive ia(ss);
        ia >> s;
    }
};

// Counter of exposed types, used for naming them.
extern std::size_t exposed_types_counter;

// This is a replacement for std::to_string that guarantees that the input n is formatted
// according to the C locale. std::to_string respects the locale settings, so in principle it could
// produce a string representation of n containing funny stuff (commas maybe?) that would make it
// unusuable to build a valid Python identifier.
inline std::string to_c_locale_string(std::size_t n)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << n;
    return oss.str();
}

// Expose class with a default constructor, and give it an implementation-defined name
// in Python guaranteed to be unique.
template <typename T>
inline bp::class_<T> expose_class()
{
    bp::class_<T> class_inst(("_exposed_type_" + to_c_locale_string(exposed_types_counter)).c_str(), bp::init<>());
    ++exposed_types_counter;
    return class_inst;
}

// for_each tuple algorithm.
template <typename Tuple, typename Op, std::size_t B = 0u, std::size_t E = std::tuple_size<Tuple>::value,
          typename = void>
static void tuple_for_each(const Tuple &t, const Op &op, typename std::enable_if<B != E>::type * = nullptr)
{
    op(std::get<B>(t));
    static_assert(B != std::numeric_limits<std::size_t>::max(), "Overflow error.");
    tuple_for_each<Tuple, Op, static_cast<std::size_t>(B + std::size_t(1)), E>(t, op);
}

template <typename Tuple, typename Op, std::size_t B = 0u, std::size_t E = std::tuple_size<Tuple>::value,
          typename = void>
static void tuple_for_each(const Tuple &, const Op &, typename std::enable_if<B == E>::type * = nullptr)
{
}

struct NullHook {
    template <typename T>
    void operator()(bp::class_<T> &) const
    {
    }
};

// Generic copy wrappers.
template <typename S>
inline S generic_copy_wrapper(const S &s)
{
    return s;
}

template <typename S>
inline S generic_deepcopy_wrapper(const S &s, bp::dict)
{
    return s;
}

// Generic evaluate wrapper.
template <typename S, typename T>
inline auto generic_evaluate_wrapper(const S &s, bp::dict dict, const T &)
    -> decltype(piranha::math::evaluate(s, std::declval<std::unordered_map<std::string, T>>()))
{
    std::unordered_map<std::string, T> cpp_dict;
    bp::stl_input_iterator<std::string> it(dict), end;
    for (; it != end; ++it) {
        cpp_dict[*it] = bp::extract<T>(dict[*it])();
    }
    return piranha::math::evaluate(s, cpp_dict);
}

// Generic lambdify wrapper.
// NOTE: need to reason about thread safety here. Lambdified objects are not thread safe,
// but separate lambdified objects could be used from different threads and we need to protect
// access to the python interpreter.
template <typename S, typename U>
inline auto generic_lambdify_wrapper(const S &s, bp::list l, bp::dict d, const U &) ->
    // NOTE: the extra map does not contribute to type determination.
    decltype(piranha::math::lambdify<U>(s, {}))
{
    // First extract the names.
    bp::stl_input_iterator<std::string> it_l(l), end_l;
    std::vector<std::string> names(it_l, end_l);
    // Next the extra map.
    bp::object deepcopy = bp::import("copy").attr("deepcopy");
    using l_type = decltype(piranha::math::lambdify<U>(s, names));
    using em_type = typename l_type::extra_map_type;
    em_type extra_map;
    bp::stl_input_iterator<std::string> it_d(d), end_d;
    for (; it_d != end_d; ++it_d) {
        // Get the string.
        std::string str = *it_d;
        // Make a deep copy of the mapped function.
        bp::object f_copy = deepcopy(bp::object(d[str]));
        // Write a wrapper for the copy of the mapped function.
        auto cpp_func = [f_copy](const std::vector<U> &v) -> U {
            // We will transform the input vector into a list before
            // feeding it into the Python function.
            // NOTE: here probably a NumPy array would be better.
            bp::list tmp;
            for (const auto &value : v) {
                tmp.append(value);
            }
            // Execute the Python function and try to extract the
            // return value of type U.
            return bp::extract<U>(f_copy(tmp));
        };
        // Map s to cpp_func.
        extra_map.emplace(std::move(str), std::move(cpp_func));
    }
    return piranha::math::lambdify<U>(s, names, extra_map);
}

template <typename T, typename U>
inline auto lambdified_call_operator(piranha::math::lambdified<T, U> &l, bp::object o) -> decltype(l({}))
{
    bp::stl_input_iterator<U> it(o), end;
    std::vector<U> values(it, end);
    return l(values);
}

template <typename T, typename U>
inline std::string lambdified_repr(const piranha::math::lambdified<T, U> &l)
{
    std::ostringstream oss;
    oss << "Lambdified object: " << l.get_evaluable() << '\n';
    oss << "Evaluation variables: [";
    const auto size = l.get_names().size();
    for (decltype(l.get_names().size()) i = 0u; i < size; ++i) {
        oss << '"' << l.get_names()[i] << '"';
        if (i != size - 1u) {
            oss << ',';
        }
    }
    oss << "]\n";
    oss << "Symbols in the extra map: [";
    const auto en = l.get_extra_names();
    for (decltype(en.size()) i = 0u; i < en.size(); ++i) {
        oss << '"' << en[i] << '"';
        if (i != en.size() - 1u) {
            oss << ',';
        }
    }
    oss << ']';
    return oss.str();
}

// Generic wrapper for the lambdified class.
extern std::size_t lambdified_counter;

template <typename S, typename U>
inline void generic_expose_lambdified()
{
    using l_type = piranha::math::lambdified<S, U>;
    bp::class_<l_type> class_inst(("_lambdified_" + to_c_locale_string(lambdified_counter)).c_str(), bp::no_init);
    // Expose copy/deepcopy.
    class_inst.def("__copy__", generic_copy_wrapper<l_type>);
    class_inst.def("__deepcopy__", generic_deepcopy_wrapper<l_type>);
    // The call operator.
    class_inst.def("__call__", lambdified_call_operator<S, U>);
    // The repr.
    class_inst.def("__repr__", lambdified_repr<S, U>);
    // Update the exposition counter.
    ++lambdified_counter;
}

// Generic canonical transformation wrapper.
// NOTE: last param is dummy to let the Boost.Python type system to pick the correct type.
template <typename S>
inline bool generic_canonical_wrapper(bp::list new_p, bp::list new_q, bp::list p_list, bp::list q_list, const S &)
{
    bp::stl_input_iterator<S> begin_new_p(new_p), end_new_p;
    bp::stl_input_iterator<S> begin_new_q(new_q), end_new_q;
    bp::stl_input_iterator<std::string> begin_p(p_list), end_p;
    bp::stl_input_iterator<std::string> begin_q(q_list), end_q;
    return piranha::math::transformation_is_canonical(
        std::vector<S>(begin_new_p, end_new_p), std::vector<S>(begin_new_q, end_new_q),
        std::vector<std::string>(begin_p, end_p), std::vector<std::string>(begin_q, end_q));
}

// Generic Poisson bracket wrapper.
template <typename S>
inline auto generic_pbracket_wrapper(const S &s1, const S &s2, bp::list p_list, bp::list q_list)
    -> decltype(piranha::math::pbracket(s1, s2, {}, {}))
{
    bp::stl_input_iterator<std::string> begin_p(p_list), end_p;
    bp::stl_input_iterator<std::string> begin_q(q_list), end_q;
    return piranha::math::pbracket(s1, s2, std::vector<std::string>(begin_p, end_p),
                                   std::vector<std::string>(begin_q, end_q));
}

// Generic degree wrappers.
template <typename S>
inline auto generic_degree_wrapper(const S &s) -> decltype(piranha::math::degree(s))
{
    return piranha::math::degree(s);
}

template <typename S>
inline auto generic_partial_degree_wrapper(const S &s, bp::list l)
    -> decltype(piranha::math::degree(s, std::vector<std::string>{}))
{
    bp::stl_input_iterator<std::string> begin(l), end;
    return piranha::math::degree(s, std::vector<std::string>(begin, end));
}

template <typename S>
inline auto generic_ldegree_wrapper(const S &s) -> decltype(piranha::math::ldegree(s))
{
    return piranha::math::ldegree(s);
}

template <typename S>
inline auto generic_partial_ldegree_wrapper(const S &s, bp::list l)
    -> decltype(piranha::math::ldegree(s, std::vector<std::string>{}))
{
    bp::stl_input_iterator<std::string> begin(l), end;
    return piranha::math::ldegree(s, std::vector<std::string>(begin, end));
}

// Generic latex representation wrapper.
template <typename S>
inline std::string generic_latex_wrapper(const S &s)
{
    std::ostringstream oss;
    s.print_tex(oss);
    return oss.str();
}

// A simple wrapper for in-place division. We need this because Boost.Python does not expose correctly
// in-place division in Python 3.
// https://svn.boost.org/trac/boost/ticket/11797
template <typename T, typename U>
inline T &generic_in_place_division_wrapper(T &n, const U &d)
{
    return n /= d;
}

// Utility function to check if object is callable. Will throw TypeError if not.
inline void check_callable(bp::object func)
{
#if PY_MAJOR_VERSION < 3
    bp::object builtin_module = bp::import("__builtin__");
    if (!builtin_module.attr("callable")(func)) {
        ::PyErr_SetString(PyExc_TypeError, "object is not callable");
        bp::throw_error_already_set();
    }
#else
    // This will throw on failure.
    try {
        bp::object call_method = func.attr("__call__");
        (void)call_method;
    } catch (...) {
        // NOTE: it seems like it is ok to overwrite the global error status of Python here,
        // after it has already been set by Boost.Python via the exception thrown above.
        ::PyErr_SetString(PyExc_TypeError, "object is not callable");
        bp::throw_error_already_set();
    }
#endif
}

// Various generic utils for differentiation.
// NOTE: here it is important to keep the member/free distinction because of the special semantics of partial.
template <typename S>
inline auto generic_partial_wrapper(const S &s, const std::string &name) -> decltype(piranha::math::partial(s, name))
{
    return piranha::math::partial(s, name);
}

template <typename S>
inline auto generic_partial_member_wrapper(const S &s, const std::string &name) -> decltype(s.partial(name))
{
    return s.partial(name);
}

// NOTE: here we need to take care of multithreading in the future. We need to check at least that:
// - the check and copy are thread-safe,
// - calling the function itself is thread-safe (e.g., if pbracket one day gets parallelised we might
//   end up calling func from multiple C++ threads at the same time).
// Note that the issue here is calling the same Python interpreter from multiple C++ threads of which
// the interpreter knows nothing.
template <typename S>
inline void generic_register_custom_derivative_wrapper(const std::string &name, bp::object func)
{
    using partial_type = decltype(std::declval<const S &>().partial(std::string()));
    check_callable(func);
    // Make a deep copy.
    bp::object deepcopy = bp::import("copy").attr("deepcopy");
    bp::object f_copy = deepcopy(func);
    S::register_custom_derivative(
        name, [f_copy](const S &s) -> partial_type { return bp::extract<partial_type>(f_copy(s)); });
}

// Generic s11n exposition.
template <typename S>
inline void expose_s11n(bp::class_<S> &)
{
    bp::def("_save_file", +[](const S &x, const std::string &filename, piranha::data_format f, piranha::compression c) {
        piranha::save_file(x, filename, f, c);
    });
    bp::def("_save_file", +[](const S &x, const std::string &filename) { piranha::save_file(x, filename); });
    bp::def("_load_file", +[](S &x, const std::string &filename, piranha::data_format f, piranha::compression c) {
        piranha::load_file(x, filename, f, c);
    });
    bp::def("_load_file", +[](S &x, const std::string &filename) { piranha::load_file(x, filename); });
}

// Generic series exposer.
template <template <typename...> class Series, typename Descriptor, std::size_t Begin = 0u,
          std::size_t End = std::tuple_size<typename Descriptor::params>::value, typename CustomHook = NullHook>
class series_exposer
{
    using params = typename Descriptor::params;
    // Detect the presence of interoperable types.
    PIRANHA_DECLARE_HAS_TYPEDEF(interop_types);
    // Detect the presence of pow types.
    PIRANHA_DECLARE_HAS_TYPEDEF(pow_types);
    // Detect the presence of evaluation types.
    PIRANHA_DECLARE_HAS_TYPEDEF(eval_types);
    // Detect the presence of substitution types.
    PIRANHA_DECLARE_HAS_TYPEDEF(subs_types);
    // Detect the presence of degree truncation types.
    PIRANHA_DECLARE_HAS_TYPEDEF(degree_truncation_types);
    // Expose constructor conditionally.
    template <typename U, typename T>
    static void expose_ctor(bp::class_<T> &cl,
                            typename std::enable_if<std::is_constructible<T, U>::value>::type * = nullptr)
    {
        cl.def(bp::init<U>());
    }
    template <typename U, typename T>
    static void expose_ctor(bp::class_<T> &,
                            typename std::enable_if<!std::is_constructible<T, U>::value>::type * = nullptr)
    {
    }
    // Sparsity wrapper.
    template <typename S>
    static bp::dict table_sparsity_wrapper(const S &s)
    {
        const auto tmp = s.table_sparsity();
        bp::dict retval;
        for (const auto &p : tmp) {
            retval[p.first] = p.second;
        }
        return retval;
    }
    // Wrapper to list.
    template <typename S>
    static bp::list to_list_wrapper(const S &s)
    {
        bp::list retval;
        for (const auto &t : s) {
            retval.append(bp::make_tuple(t.first, t.second));
        }
        return retval;
    }
    // Expose arithmetics operations with another type, if supported.
    template <typename S, typename T>
    using common_ops_ic = std::
        integral_constant<bool,
                          piranha::is_addable_in_place<S, T>::value && piranha::is_addable<S, T>::value
                              && piranha::is_addable<T, S>::value && piranha::is_subtractable_in_place<S, T>::value
                              && piranha::is_subtractable<S, T>::value && piranha::is_subtractable<T, S>::value
                              && piranha::is_multipliable_in_place<S, T>::value && piranha::is_multipliable<S, T>::value
                              && piranha::is_multipliable<T, S>::value && piranha::is_equality_comparable<T, S>::value
                              && piranha::is_equality_comparable<S, T>::value>;
    template <typename S, typename T, typename std::enable_if<common_ops_ic<S, T>::value, int>::type = 0>
    static void expose_common_ops(bp::class_<S> &series_class, const T &in)
    {
        namespace sn = boost::python::self_ns;
        series_class.def(sn::operator+=(bp::self, in));
        series_class.def(sn::operator+(bp::self, in));
        series_class.def(sn::operator+(in, bp::self));
        series_class.def(sn::operator-=(bp::self, in));
        series_class.def(sn::operator-(bp::self, in));
        series_class.def(sn::operator-(in, bp::self));
        series_class.def(sn::operator*=(bp::self, in));
        series_class.def(sn::operator*(bp::self, in));
        series_class.def(sn::operator*(in, bp::self));
        series_class.def(sn::operator==(bp::self, in));
        series_class.def(sn::operator==(in, bp::self));
        series_class.def(sn::operator!=(bp::self, in));
        series_class.def(sn::operator!=(in, bp::self));
    }
    template <typename S, typename T, typename std::enable_if<!common_ops_ic<S, T>::value, int>::type = 0>
    static void expose_common_ops(bp::class_<S> &, const T &)
    {
    }
    // Handle division separately.
    template <typename S, typename T>
    using division_ops_ic
        = std::integral_constant<bool, piranha::is_divisible_in_place<S, T>::value && piranha::is_divisible<S, T>::value
                                           && piranha::is_divisible<T, S>::value>;
    template <typename S, typename T, typename std::enable_if<division_ops_ic<S, T>::value, int>::type = 0>
    static void expose_division(bp::class_<S> &series_class, const T &in)
    {
        namespace sn = boost::python::self_ns;
#if PY_MAJOR_VERSION < 3
        series_class.def(sn::operator/=(bp::self, in));
#else
        series_class.def("__itruediv__", generic_in_place_division_wrapper<S, T>, bp::return_arg<1u>{});
#endif
        series_class.def(sn::operator/(bp::self, in));
        series_class.def(sn::operator/(in, bp::self));
    }
    template <typename S, typename T, typename std::enable_if<!division_ops_ic<S, T>::value, int>::type = 0>
    static void expose_division(bp::class_<S> &, const T &)
    {
    }
    // Main wrapper.
    template <typename T, typename S>
    static void expose_arithmetics(bp::class_<S> &series_class)
    {
        T in;
        expose_common_ops(series_class, in);
        expose_division(series_class, in);
    }
    // Exponentiation support.
    template <typename S>
    struct pow_exposer {
        pow_exposer(bp::class_<S> &series_class) : m_series_class(series_class)
        {
        }
        bp::class_<S> &m_series_class;
        template <typename T, typename U>
        static auto pow_wrapper(const T &s, const U &x) -> decltype(piranha::math::pow(s, x))
        {
            return piranha::math::pow(s, x);
        }
        template <typename T>
        void operator()(const T &,
                        typename std::enable_if<piranha::is_exponentiable<S, T>::value>::type * = nullptr) const
        {
            m_series_class.def("__pow__", pow_wrapper<S, T>);
        }
        template <typename T>
        void operator()(const T &,
                        typename std::enable_if<!piranha::is_exponentiable<S, T>::value>::type * = nullptr) const
        {
        }
    };
    template <typename S, typename T = Descriptor>
    static void expose_pow(bp::class_<S> &series_class,
                           typename std::enable_if<has_typedef_pow_types<T>::value>::type * = nullptr)
    {
        using pow_types = typename Descriptor::pow_types;
        pow_types pt;
        tuple_for_each(pt, pow_exposer<S>(series_class));
    }
    template <typename S, typename T = Descriptor>
    static void expose_pow(bp::class_<S> &, typename std::enable_if<!has_typedef_pow_types<T>::value>::type * = nullptr)
    {
    }
    // Evaluation.
    template <typename S>
    struct eval_exposer {
        eval_exposer(bp::class_<S> &series_class) : m_series_class(series_class)
        {
        }
        bp::class_<S> &m_series_class;
        template <typename T>
        void operator()(const T &, typename std::enable_if<piranha::is_evaluable<S, T>::value>::type * = nullptr) const
        {
            m_series_class.def("_evaluate", generic_evaluate_wrapper<S, T>);
            bp::def("_evaluate", generic_evaluate_wrapper<S, T>);
            bp::def("_lambdify", generic_lambdify_wrapper<S, T>);
            generic_expose_lambdified<S, T>();
        }
        template <typename T>
        void operator()(const T &, typename std::enable_if<!piranha::is_evaluable<S, T>::value>::type * = nullptr) const
        {
        }
    };
    template <typename S, typename T = Descriptor>
    static void expose_eval(bp::class_<S> &series_class,
                            typename std::enable_if<has_typedef_eval_types<T>::value>::type * = nullptr)
    {
        using eval_types = typename Descriptor::eval_types;
        eval_types et;
        tuple_for_each(et, eval_exposer<S>(series_class));
    }
    template <typename S, typename T = Descriptor>
    static void expose_eval(bp::class_<S> &,
                            typename std::enable_if<!has_typedef_eval_types<T>::value>::type * = nullptr)
    {
    }
    // Substitution.
    template <typename S>
    struct subs_exposer {
        subs_exposer(bp::class_<S> &series_class) : m_series_class(series_class)
        {
        }
        bp::class_<S> &m_series_class;
        template <typename T>
        void operator()(const T &x) const
        {
            impl_subs(x);
            impl_ipow_subs(x);
            impl_t_subs(x);
        }
        template <typename T, typename std::enable_if<piranha::has_subs<S, T>::value, int>::type = 0>
        void impl_subs(const T &) const
        {
            m_series_class.def("subs", subs_wrapper<T>);
            bp::def("_subs", subs_wrapper<T>);
        }
        template <typename T, typename std::enable_if<!piranha::has_subs<S, T>::value, int>::type = 0>
        void impl_subs(const T &) const
        {
        }
        template <typename T, typename std::enable_if<piranha::has_ipow_subs<S, T>::value, int>::type = 0>
        void impl_ipow_subs(const T &) const
        {
            m_series_class.def("ipow_subs", ipow_subs_wrapper<T>);
            bp::def("_ipow_subs", ipow_subs_wrapper<T>);
        }
        template <typename T, typename std::enable_if<!piranha::has_ipow_subs<S, T>::value, int>::type = 0>
        void impl_ipow_subs(const T &) const
        {
        }
        template <typename T, typename std::enable_if<piranha::has_t_subs<S, T, T>::value, int>::type = 0>
        void impl_t_subs(const T &) const
        {
            m_series_class.def("t_subs", t_subs_wrapper<T>);
            bp::def("_t_subs", t_subs_wrapper<T>);
        }
        template <typename T, typename std::enable_if<!piranha::has_t_subs<S, T, T>::value, int>::type = 0>
        void impl_t_subs(const T &) const
        {
        }
        // The actual wrappers.
        template <typename T>
        static auto subs_wrapper(const S &s, const std::string &name, const T &x) -> decltype(s.subs(name, x))
        {
            return s.subs(name, x);
        }
        template <typename T>
        static auto ipow_subs_wrapper(const S &s, const std::string &name, const piranha::integer &n, const T &x)
            -> decltype(s.ipow_subs(name, n, x))
        {
            return s.ipow_subs(name, n, x);
        }
        template <typename T>
        static auto t_subs_wrapper(const S &s, const std::string &name, const T &x, const T &y)
            -> decltype(s.t_subs(name, x, y))
        {
            return s.t_subs(name, x, y);
        }
    };
    template <typename S, typename T = Descriptor>
    static void expose_subs(bp::class_<S> &series_class,
                            typename std::enable_if<has_typedef_subs_types<T>::value>::type * = nullptr)
    {
        using subs_types = typename Descriptor::subs_types;
        subs_types st;
        tuple_for_each(st, subs_exposer<S>(series_class));
        // Implement subs with self.
        S tmp;
        subs_exposer<S> se(series_class);
        se(tmp);
    }
    template <typename S, typename T = Descriptor>
    static void expose_subs(bp::class_<S> &,
                            typename std::enable_if<!has_typedef_subs_types<T>::value>::type * = nullptr)
    {
    }
    // Interaction with interoperable types.
    template <typename S>
    struct interop_exposer {
        interop_exposer(bp::class_<S> &series_class) : m_series_class(series_class)
        {
        }
        bp::class_<S> &m_series_class;
        template <typename T>
        void operator()(const T &) const
        {
            expose_ctor<const T &>(m_series_class);
            expose_arithmetics<T>(m_series_class);
        }
    };
    // Here we have three implementations of this function. The final objective is to enable construction and
    // arithmetics
    // with respect to the coefficient type, and, recursively, the coefficient type of the coefficient type.
    // 1. Series2 is a series type whose cf is not among interoperable types.
    // In this case, we expose the interoperability with the cf, and we try to expose also the interoperability with the
    // coefficient
    // type of cf, if it is a series type.
    template <typename InteropTypes, typename Series2, typename S>
    static void
    expose_cf_interop(bp::class_<S> &series_class,
                      typename std::enable_if<!piranha::detail::type_in_tuple<typename Series2::term_type::cf_type,
                                                                              InteropTypes>::value>::type * = nullptr)
    {
        using cf_type = typename Series2::term_type::cf_type;
        cf_type cf;
        interop_exposer<S> ie(series_class);
        ie(cf);
        expose_cf_interop<InteropTypes, cf_type>(series_class);
    }
    // 2. Series2 is a series type whose cf is among the interoperable types. Try to go deeper in the hierarchy.
    template <typename InteropTypes, typename Series2, typename S>
    static void
    expose_cf_interop(bp::class_<S> &series_class,
                      typename std::enable_if<piranha::detail::type_in_tuple<typename Series2::term_type::cf_type,
                                                                             InteropTypes>::value>::type * = nullptr)
    {
        expose_cf_interop<InteropTypes, typename Series2::term_type::cf_type>(series_class);
    }
    // 3. Series2 is not a series type. This signals the end of the recursion.
    template <typename InteropTypes, typename Series2, typename S>
    static void expose_cf_interop(bp::class_<S> &,
                                  typename std::enable_if<!piranha::is_series<Series2>::value>::type * = nullptr)
    {
    }
    template <typename S, typename T = Descriptor>
    static void expose_interoperable(bp::class_<S> &series_class,
                                     typename std::enable_if<has_typedef_interop_types<T>::value>::type * = nullptr)
    {
        using interop_types = typename Descriptor::interop_types;
        interop_types it;
        tuple_for_each(it, interop_exposer<S>(series_class));
        // Interoperate conditionally with coefficient type, if it is not already in the
        // list of interoperable types.
        expose_cf_interop<interop_types, S>(series_class);
    }
    template <typename S, typename T = Descriptor>
    static void expose_interoperable(bp::class_<S> &,
                                     typename std::enable_if<!has_typedef_interop_types<T>::value>::type * = nullptr)
    {
    }
    // Expose integration conditionally.
    template <typename S>
    static auto integrate_wrapper(const S &s, const std::string &name) -> decltype(piranha::math::integrate(s, name))
    {
        return piranha::math::integrate(s, name);
    }
    template <typename S>
    static void expose_integrate(bp::class_<S> &series_class,
                                 typename std::enable_if<piranha::is_integrable<S>::value>::type * = nullptr)
    {
        series_class.def("integrate", integrate_wrapper<S>);
        bp::def("_integrate", integrate_wrapper<S>);
    }
    template <typename S>
    static void expose_integrate(bp::class_<S> &,
                                 typename std::enable_if<!piranha::is_integrable<S>::value>::type * = nullptr)
    {
    }
    // Differentiation.
    template <typename S>
    static void expose_partial(bp::class_<S> &series_class,
                               typename std::enable_if<piranha::is_differentiable<S>::value>::type * = nullptr)
    {
        // NOTE: we need this below in order to specify exactly the address of the templated
        // static method for unregistering the custom derivatives.
        using partial_type = decltype(piranha::math::partial(std::declval<const S &>(), std::string{}));
        series_class.def("partial", generic_partial_member_wrapper<S>);
        bp::def("_partial", generic_partial_wrapper<S>);
        // Custom derivatives support.
        series_class.def("register_custom_derivative", generic_register_custom_derivative_wrapper<S>)
            .staticmethod("register_custom_derivative");
        series_class.def("unregister_custom_derivative", S::template unregister_custom_derivative<S, partial_type>)
            .staticmethod("unregister_custom_derivative");
        series_class
            .def("unregister_all_custom_derivatives", S::template unregister_all_custom_derivatives<S, partial_type>)
            .staticmethod("unregister_all_custom_derivatives");
    }
    template <typename S>
    static void expose_partial(bp::class_<S> &,
                               typename std::enable_if<!piranha::is_differentiable<S>::value>::type * = nullptr)
    {
    }
    template <typename S>
    static void expose_pbracket(bp::class_<S> &,
                                typename std::enable_if<piranha::has_pbracket<S>::value>::type * = nullptr)
    {
        bp::def("_pbracket", generic_pbracket_wrapper<S>);
    }
    template <typename S>
    static void expose_pbracket(bp::class_<S> &,
                                typename std::enable_if<!piranha::has_pbracket<S>::value>::type * = nullptr)
    {
    }
    template <typename S>
    static void
    expose_canonical(bp::class_<S> &,
                     typename std::enable_if<piranha::has_transformation_is_canonical<S>::value>::type * = nullptr)
    {
        bp::def("_transformation_is_canonical", generic_canonical_wrapper<S>);
    }
    template <typename S>
    static void
    expose_canonical(bp::class_<S> &,
                     typename std::enable_if<!piranha::has_transformation_is_canonical<S>::value>::type * = nullptr)
    {
    }
    // filter() wrap.
    template <typename S>
    static S wrap_filter(const S &s, bp::object func)
    {
        typedef typename S::term_type::cf_type cf_type;
        check_callable(func);
        auto cpp_func = [func](const std::pair<cf_type, S> &p) -> bool {
            return bp::extract<bool>(func(bp::make_tuple(p.first, p.second)));
        };
        return s.filter(cpp_func);
    }
    // Check if type is tuple with two elements (for use in wrap_transform).
    static void check_tuple_2(bp::object obj)
    {
        bp::object builtin_module = bp::import(
#if PY_MAJOR_VERSION < 3
            "__builtin__"
#else
            "builtins"
#endif
            );
        bp::object isinstance = builtin_module.attr("isinstance");
        bp::object tuple_type = builtin_module.attr("tuple");
        if (!isinstance(obj, tuple_type)) {
            ::PyErr_SetString(PyExc_TypeError, "object is not a tuple");
            bp::throw_error_already_set();
        }
        const std::size_t len = bp::extract<std::size_t>(obj.attr("__len__")());
        if (len != 2u) {
            ::PyErr_SetString(PyExc_ValueError,
                              "the tuple to be returned in series transformation must have 2 elements");
            bp::throw_error_already_set();
        }
    }
    // transform() wrap.
    template <typename S>
    static S wrap_transform(const S &s, bp::object func)
    {
        typedef typename S::term_type::cf_type cf_type;
        check_callable(func);
        auto cpp_func = [func](const std::pair<cf_type, S> &p) -> std::pair<cf_type, S> {
            bp::object tmp = func(bp::make_tuple(p.first, p.second));
            check_tuple_2(tmp);
            cf_type tmp_cf = bp::extract<cf_type>(tmp[0]);
            S tmp_key = bp::extract<S>(tmp[1]);
            return std::make_pair(std::move(tmp_cf), std::move(tmp_key));
        };
        return s.transform(cpp_func);
    }
    // Sin and cos.
    template <typename S>
    static auto sin_wrapper(const S &s) -> decltype(piranha::math::sin(s))
    {
        return piranha::math::sin(s);
    }
    template <typename S>
    static auto cos_wrapper(const S &s) -> decltype(piranha::math::cos(s))
    {
        return piranha::math::cos(s);
    }
    template <typename S>
    static void expose_sin_cos(
        typename std::enable_if<piranha::has_sine<S>::value && piranha::has_cosine<S>::value>::type * = nullptr)
    {
        bp::def("_sin", sin_wrapper<S>);
        bp::def("_cos", cos_wrapper<S>);
    }
    template <typename S>
    static void expose_sin_cos(
        typename std::enable_if<!piranha::has_sine<S>::value || !piranha::has_cosine<S>::value>::type * = nullptr)
    {
    }
    // Power series exposer.
    template <typename S>
    static void expose_power_series(bp::class_<S> &series_class)
    {
        expose_degree(series_class);
        expose_degree_truncation(series_class);
    }
    template <typename T>
    static void expose_degree(
        bp::class_<T> &,
        typename std::enable_if<piranha::has_degree<T>::value && piranha::has_ldegree<T>::value>::type * = nullptr)
    {
        bp::def("_degree", generic_degree_wrapper<T>);
        bp::def("_degree", generic_partial_degree_wrapper<T>);
        bp::def("_ldegree", generic_ldegree_wrapper<T>);
        bp::def("_ldegree", generic_partial_ldegree_wrapper<T>);
    }
    template <typename T>
    static void expose_degree(
        bp::class_<T> &,
        typename std::enable_if<!piranha::has_degree<T>::value || !piranha::has_ldegree<T>::value>::type * = nullptr)
    {
    }
    // Truncation.
    template <typename S>
    struct truncate_degree_exposer {
        truncate_degree_exposer(bp::class_<S> &series_class) : m_series_class(series_class)
        {
        }
        bp::class_<S> &m_series_class;
        template <typename T>
        static S truncate_degree_wrapper(const S &s, const T &x)
        {
            return s.truncate_degree(x);
        }
        template <typename T>
        static S truncate_pdegree_wrapper(const S &s, const T &x, bp::list l)
        {
            bp::stl_input_iterator<std::string> begin(l), end;
            return s.truncate_degree(x, std::vector<std::string>(begin, end));
        }
        template <typename T>
        void expose_truncate_degree(
            const T &, typename std::enable_if<piranha::has_truncate_degree<S, T>::value>::type * = nullptr) const
        {
            // Expose both as member function and free function.
            m_series_class.def("truncate_degree", truncate_degree_wrapper<T>);
            bp::def("_truncate_degree", truncate_degree_wrapper<T>);
            // The partial version.
            m_series_class.def("truncate_degree", truncate_pdegree_wrapper<T>);
            bp::def("_truncate_degree", truncate_pdegree_wrapper<T>);
        }
        template <typename T>
        void expose_truncate_degree(
            const T &, typename std::enable_if<!piranha::has_truncate_degree<S, T>::value>::type * = nullptr) const
        {
        }
        template <typename T>
        void operator()(const T &x) const
        {
            expose_truncate_degree(x);
        }
    };
    template <typename S, typename T = Descriptor,
              typename std::enable_if<has_typedef_degree_truncation_types<T>::value, int>::type = 0>
    static void expose_degree_truncation(bp::class_<S> &series_class)
    {
        using dt_types = typename Descriptor::degree_truncation_types;
        dt_types dt;
        tuple_for_each(dt, truncate_degree_exposer<S>(series_class));
    }
    template <typename S, typename T = Descriptor,
              typename std::enable_if<!has_typedef_degree_truncation_types<T>::value, int>::type = 0>
    static void expose_degree_truncation(bp::class_<S> &)
    {
    }
    // Trigonometric exposer.
    template <typename S>
    static void expose_trigonometric_series(
        bp::class_<S> &,
        typename std::enable_if<piranha::has_t_degree<S>::value && piranha::has_t_ldegree<S>::value
                                && piranha::has_t_order<S>::value && piranha::has_t_lorder<S>::value>::type * = nullptr)
    {
        bp::def("_t_degree", wrap_t_degree<S>);
        bp::def("_t_degree", wrap_partial_t_degree<S>);
        bp::def("_t_ldegree", wrap_t_ldegree<S>);
        bp::def("_t_ldegree", wrap_partial_t_ldegree<S>);
        bp::def("_t_order", wrap_t_order<S>);
        bp::def("_t_order", wrap_partial_t_order<S>);
        bp::def("_t_lorder", wrap_t_lorder<S>);
        bp::def("_t_lorder", wrap_partial_t_lorder<S>);
    }
    template <typename S>
    static void expose_trigonometric_series(
        bp::class_<S> &, typename std::enable_if<!piranha::has_t_degree<S>::value || !piranha::has_t_ldegree<S>::value
                                                 || !piranha::has_t_order<S>::value
                                                 || !piranha::has_t_lorder<S>::value>::type * = nullptr)
    {
    }
    template <typename S>
    static auto wrap_t_degree(const S &s) -> decltype(s.t_degree())
    {
        return s.t_degree();
    }
    template <typename S>
    static auto wrap_partial_t_degree(const S &s, bp::list l) -> decltype(s.t_degree(std::vector<std::string>{}))
    {
        bp::stl_input_iterator<std::string> begin(l), end;
        return s.t_degree(std::vector<std::string>(begin, end));
    }
    template <typename S>
    static auto wrap_t_ldegree(const S &s) -> decltype(s.t_ldegree())
    {
        return s.t_ldegree();
    }
    template <typename S>
    static auto wrap_partial_t_ldegree(const S &s, bp::list l) -> decltype(s.t_ldegree(std::vector<std::string>{}))
    {
        bp::stl_input_iterator<std::string> begin(l), end;
        return s.t_ldegree(std::vector<std::string>(begin, end));
    }
    template <typename S>
    static auto wrap_t_order(const S &s) -> decltype(s.t_order())
    {
        return s.t_order();
    }
    template <typename S>
    static auto wrap_partial_t_order(const S &s, bp::list l) -> decltype(s.t_order(std::vector<std::string>{}))
    {
        bp::stl_input_iterator<std::string> begin(l), end;
        return s.t_order(std::vector<std::string>(begin, end));
    }
    template <typename S>
    static auto wrap_t_lorder(const S &s) -> decltype(s.t_lorder())
    {
        return s.t_lorder();
    }
    template <typename S>
    static auto wrap_partial_t_lorder(const S &s, bp::list l) -> decltype(s.t_lorder(std::vector<std::string>{}))
    {
        bp::stl_input_iterator<std::string> begin(l), end;
        return s.t_lorder(std::vector<std::string>(begin, end));
    }
    // Symbol set wrapper.
    template <typename S>
    static bp::list symbol_set_wrapper(const S &s)
    {
        bp::list retval;
        for (auto it = s.get_symbol_set().begin(); it != s.get_symbol_set().end(); ++it) {
            retval.append(it->get_name());
        }
        return retval;
    }
    // invert().
    template <typename S>
    static auto invert_wrapper(const S &s) -> decltype(piranha::math::invert(s))
    {
        return piranha::math::invert(s);
    }
    template <typename S, typename std::enable_if<piranha::is_invertible<S>::value, int>::type = 0>
    static void expose_invert(bp::class_<S> &)
    {
        bp::def("_invert", invert_wrapper<S>);
    }
    template <typename S, typename std::enable_if<!piranha::is_invertible<S>::value, int>::type = 0>
    static void expose_invert(bp::class_<S> &)
    {
    }
    // Main exposer.
    struct exposer_op {
        explicit exposer_op() = default;
        template <typename... Args>
        void operator()(const std::tuple<Args...> &) const
        {
            using s_type = Series<Args...>;
            // Start exposing.
            auto series_class = expose_class<s_type>();
            // Connect the Python type to the C++ type.
            register_exposed_type(series_class);
            // Register the template instance corresponding to the series, so that we can
            // fetch its type generator via the type system machinery.
            register_template_instance<Series, Args...>();
            // Add the _is_exposed_pyranha_type tag.
            series_class.attr("_is_exposed_pyranha_type") = true;
            // Constructor from string, if available.
            expose_ctor<const std::string &>(series_class);
            // Copy constructor.
            series_class.def(bp::init<const s_type &>());
            // Shallow and deep copy.
            series_class.def("__copy__", generic_copy_wrapper<s_type>);
            series_class.def("__deepcopy__", generic_deepcopy_wrapper<s_type>);
            // NOTE: here repr is found via argument-dependent lookup.
            series_class.def(repr(bp::self));
            // Length.
            series_class.def("__len__", &s_type::size);
            // Table properties.
            series_class.def("table_load_factor", &s_type::table_load_factor);
            series_class.def("table_bucket_count", &s_type::table_bucket_count);
            series_class.def("table_sparsity", table_sparsity_wrapper<s_type>);
            // Conversion to list.
            series_class.add_property("list", to_list_wrapper<s_type>);
            // Interaction with self.
            series_class.def(bp::self += bp::self);
            series_class.def(bp::self + bp::self);
            series_class.def(bp::self -= bp::self);
            series_class.def(bp::self - bp::self);
            series_class.def(bp::self *= bp::self);
            series_class.def(bp::self * bp::self);
#if PY_MAJOR_VERSION < 3
            series_class.def(bp::self /= bp::self);
#else
            series_class.def("__itruediv__", generic_in_place_division_wrapper<s_type, s_type>, bp::return_arg<1u>{});
#endif
            series_class.def(bp::self / bp::self);
            series_class.def(bp::self == bp::self);
            series_class.def(bp::self != bp::self);
            series_class.def(+bp::self);
            series_class.def(-bp::self);
            // NOTE: here this method is available if is_identical() is (that is, if the series are comparable), so
            // put it here - even if logically it belongs to exponentiation. We assume the series are comparable anyway.
            series_class.def("clear_pow_cache", s_type::template clear_pow_cache<s_type, 0>)
                .staticmethod("clear_pow_cache");
            // Expose interoperable types.
            expose_interoperable(series_class);
            // Expose pow.
            expose_pow(series_class);
            // Evaluate.
            expose_eval(series_class);
            // Subs.
            expose_subs(series_class);
            // Integration.
            expose_integrate(series_class);
            // Partial differentiation.
            expose_partial(series_class);
            // Poisson bracket.
            expose_pbracket(series_class);
            // Canonical test.
            expose_canonical(series_class);
            // Filter and transform.
            series_class.def("filter", wrap_filter<s_type>);
            series_class.def("transform", wrap_transform<s_type>);
            // Trimming.
            series_class.def("trim", &s_type::trim);
            // Sin and cos.
            expose_sin_cos<s_type>();
            // Power series.
            expose_power_series(series_class);
            // Trigonometric series.
            expose_trigonometric_series(series_class);
            // Latex.
            series_class.def("_latex_", generic_latex_wrapper<s_type>);
            // Arguments set.
            series_class.add_property("symbol_set", symbol_set_wrapper<s_type>);
            // Pickle support.
            series_class.def_pickle(generic_pickle_suite<s_type>());
            // Expose invert(), if present.
            expose_invert(series_class);
            // Expose s11n.
            expose_s11n(series_class);
            // Run the custom hook.
            CustomHook{}(series_class);
        }
    };

public:
    series_exposer()
    {
        // NOTE: probably we can avoid instantiating p here, look at the tuple for_each
        // in vargs_to_v_t_idx.
        params p;
        tuple_for_each<params, exposer_op, Begin, End>(p, exposer_op{});
    }
    series_exposer(const series_exposer &) = delete;
    series_exposer(series_exposer &&) = delete;
    series_exposer &operator=(const series_exposer &) = delete;
    series_exposer &operator=(series_exposer &&) = delete;
    ~series_exposer() = default;
};

inline bp::list get_exposed_types_list()
{
    bp::list retval;
    for (const auto &p : et_map) {
        // NOTE: the idea here is that in the future we might want to use the et_map
        // for other than pyranha types, so we need a way to distinguish in et_map between
        // pyranha and non-pyranha types. Currently all types in et_map are exposed pyranha types.
        if (hasattr(p.second, "_is_exposed_pyranha_type")) {
            retval.append(p.second);
        }
    }
    return retval;
}
}

#endif
