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

#include "../src/monomial.hpp"

#define BOOST_TEST_MODULE monomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <list>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../src/exceptions.hpp"
#include "../src/init.hpp"
#include "../src/key_is_convertible.hpp"
#include "../src/key_is_multipliable.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/pow.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/symbol.hpp"
#include "../src/symbol_set.hpp"
#include "../src/term.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<signed char, int, integer, rational> expo_types;
typedef boost::mpl::vector<std::integral_constant<std::size_t, 0u>,
                           std::integral_constant<std::size_t, 1u>,
                           std::integral_constant<std::size_t, 5u>,
                           std::integral_constant<std::size_t, 10u>>
    size_types;

// Constructors, assignments and element access.
struct constructor_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> monomial_type;
      BOOST_CHECK(is_key<monomial_type>::value);
      monomial_type m0;
      BOOST_CHECK_NO_THROW(monomial_type tmp = monomial_type());
      BOOST_CHECK_NO_THROW(monomial_type tmp = monomial_type(monomial_type()));
      BOOST_CHECK_NO_THROW(monomial_type tmp(m0));
      // From init list.
      monomial_type m1{T(0), T(1), T(2), T(3)};
      BOOST_CHECK_EQUAL(m1.size(), static_cast<decltype(m1.size())>(4));
      for (typename monomial_type::size_type i = 0; i < m1.size(); ++i) {
        BOOST_CHECK_EQUAL(m1[i], T(i));
        BOOST_CHECK_NO_THROW(m1[i] = static_cast<T>(T(i) + T(1)));
        BOOST_CHECK_EQUAL(m1[i], T(i) + T(1));
      }
      monomial_type m1a{0, 1, 2, 3};
      BOOST_CHECK_EQUAL(m1a.size(), static_cast<decltype(m1a.size())>(4));
      for (typename monomial_type::size_type i = 0; i < m1a.size(); ++i) {
        BOOST_CHECK_EQUAL(m1a[i], T(i));
        BOOST_CHECK_NO_THROW(m1a[i] = static_cast<T>(T(i) + T(1)));
        BOOST_CHECK_EQUAL(m1a[i], T(i) + T(1));
      }
      BOOST_CHECK_NO_THROW(m0 = m1);
      BOOST_CHECK_NO_THROW(m0 = std::move(m1));
      // From range and symbol set.
      std::vector<int> v1;
      m0 = monomial_type(v1.begin(), v1.end(), symbol_set{});
      BOOST_CHECK_EQUAL(m0.size(), 0u);
      v1 = {-1};
      m0 = monomial_type(v1.begin(), v1.end(), symbol_set{symbol{"x"}});
      BOOST_CHECK_EQUAL(m0.size(), 1u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      v1 = {-1, 2};
      m0 = monomial_type(v1.begin(), v1.end(),
                         symbol_set{symbol{"x"}, symbol{"y"}});
      BOOST_CHECK_EQUAL(m0.size(), 2u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      BOOST_CHECK_EQUAL(m0[1], 2);
      BOOST_CHECK_THROW(
          m0 = monomial_type(v1.begin(), v1.end(), symbol_set{symbol{"x"}}),
          std::invalid_argument);
      std::list<int> l1;
      m0 = monomial_type(l1.begin(), l1.end(), symbol_set{});
      BOOST_CHECK_EQUAL(m0.size(), 0u);
      l1 = {-1};
      m0 = monomial_type(l1.begin(), l1.end(), symbol_set{symbol{"x"}});
      BOOST_CHECK_EQUAL(m0.size(), 1u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      l1 = {-1, 2};
      m0 = monomial_type(l1.begin(), l1.end(),
                         symbol_set{symbol{"x"}, symbol{"y"}});
      BOOST_CHECK_EQUAL(m0.size(), 2u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      BOOST_CHECK_EQUAL(m0[1], 2);
      BOOST_CHECK_THROW(
          m0 = monomial_type(l1.begin(), l1.end(), symbol_set{symbol{"x"}}),
          std::invalid_argument);
      struct foo {};
      BOOST_CHECK((!std::is_constructible<monomial_type, foo *, foo *,
                                          symbol_set>::value));
      BOOST_CHECK((std::is_constructible<monomial_type, int *, int *,
                                         symbol_set>::value));
      // From range and symbol set.
      v1.clear();
      m0 = monomial_type(v1.begin(), v1.end());
      BOOST_CHECK_EQUAL(m0.size(), 0u);
      v1 = {-1};
      m0 = monomial_type(v1.begin(), v1.end());
      BOOST_CHECK_EQUAL(m0.size(), 1u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      v1 = {-1, 2};
      m0 = monomial_type(v1.begin(), v1.end());
      BOOST_CHECK_EQUAL(m0.size(), 2u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      BOOST_CHECK_EQUAL(m0[1], 2);
      l1.clear();
      m0 = monomial_type(l1.begin(), l1.end());
      BOOST_CHECK_EQUAL(m0.size(), 0u);
      l1 = {-1};
      m0 = monomial_type(l1.begin(), l1.end());
      BOOST_CHECK_EQUAL(m0.size(), 1u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      l1 = {-1, 2};
      m0 = monomial_type(l1.begin(), l1.end());
      BOOST_CHECK_EQUAL(m0.size(), 2u);
      BOOST_CHECK_EQUAL(m0[0], -1);
      BOOST_CHECK_EQUAL(m0[1], 2);
      BOOST_CHECK((!std::is_constructible<monomial_type, foo *, foo *>::value));
      BOOST_CHECK((std::is_constructible<monomial_type, int *, int *>::value));
      // Constructor from arguments vector.
      monomial_type m2 = monomial_type(symbol_set{});
      BOOST_CHECK_EQUAL(m2.size(), unsigned(0));
      monomial_type m3 =
          monomial_type(symbol_set({symbol("a"), symbol("b"), symbol("c")}));
      BOOST_CHECK_EQUAL(m3.size(), unsigned(3));
      symbol_set vs({symbol("a"), symbol("b"), symbol("c")});
      monomial_type k2(vs);
      BOOST_CHECK_EQUAL(k2.size(), vs.size());
      BOOST_CHECK_EQUAL(k2[0], T(0));
      BOOST_CHECK_EQUAL(k2[1], T(0));
      BOOST_CHECK_EQUAL(k2[2], T(0));
      // Generic constructor for use in series.
      BOOST_CHECK_THROW(monomial_type tmp(k2, symbol_set{}),
                        std::invalid_argument);
      monomial_type k3(k2, vs);
      BOOST_CHECK_EQUAL(k3.size(), vs.size());
      BOOST_CHECK_EQUAL(k3[0], T(0));
      BOOST_CHECK_EQUAL(k3[1], T(0));
      BOOST_CHECK_EQUAL(k3[2], T(0));
      monomial_type k4(monomial_type(vs), vs);
      BOOST_CHECK_EQUAL(k4.size(), vs.size());
      BOOST_CHECK_EQUAL(k4[0], T(0));
      BOOST_CHECK_EQUAL(k4[1], T(0));
      BOOST_CHECK_EQUAL(k4[2], T(0));
      typedef monomial<int, U> monomial_type2;
      monomial_type2 k5(vs);
      BOOST_CHECK_THROW(monomial_type tmp(k5, symbol_set{}),
                        std::invalid_argument);
      monomial_type k6(k5, vs);
      BOOST_CHECK_EQUAL(k6.size(), vs.size());
      BOOST_CHECK_EQUAL(k6[0], T(0));
      BOOST_CHECK_EQUAL(k6[1], T(0));
      BOOST_CHECK_EQUAL(k6[2], T(0));
      monomial_type k7(monomial_type2(vs), vs);
      BOOST_CHECK_EQUAL(k7.size(), vs.size());
      BOOST_CHECK_EQUAL(k7[0], T(0));
      BOOST_CHECK_EQUAL(k7[1], T(0));
      BOOST_CHECK_EQUAL(k7[2], T(0));
      // Type trait check.
      BOOST_CHECK((std::is_constructible<monomial_type, monomial_type>::value));
      BOOST_CHECK((std::is_constructible<monomial_type, symbol_set>::value));
      BOOST_CHECK((!std::is_constructible<monomial_type, std::string>::value));
      BOOST_CHECK(
          (!std::is_constructible<monomial_type, monomial_type, int>::value));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_constructor_test) {
  init();
  boost::mpl::for_each<expo_types>(constructor_tester());
}

struct hash_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> monomial_type;
      monomial_type m0;
      BOOST_CHECK_EQUAL(m0.hash(), std::size_t());
      BOOST_CHECK_EQUAL(m0.hash(), std::hash<monomial_type>()(m0));
      monomial_type m1{T(0), T(1), T(2), T(3)};
      BOOST_CHECK_EQUAL(m1.hash(), std::hash<monomial_type>()(m1));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_hash_test) {
  boost::mpl::for_each<expo_types>(hash_tester());
}

struct compatibility_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> monomial_type;
      monomial_type m0;
      BOOST_CHECK(m0.is_compatible(symbol_set{}));
      symbol_set ss({symbol("foobarize")});
      monomial_type m1{T(0), T(1)};
      BOOST_CHECK(!m1.is_compatible(ss));
      monomial_type m2{T(0)};
      BOOST_CHECK(m2.is_compatible(ss));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_compatibility_test) {
  boost::mpl::for_each<expo_types>(compatibility_tester());
}

struct ignorability_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> monomial_type;
      monomial_type m0;
      BOOST_CHECK(!m0.is_ignorable(symbol_set{}));
      monomial_type m1{T(0)};
      BOOST_CHECK(!m1.is_ignorable(symbol_set({symbol("foobarize")})));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_ignorability_test) {
  boost::mpl::for_each<expo_types>(ignorability_tester());
}

struct merge_args_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> key_type;
      symbol_set v1, v2;
      v2.add("a");
      key_type k;
      auto out = k.merge_args(v1, v2);
      BOOST_CHECK_EQUAL(out.size(), unsigned(1));
      BOOST_CHECK_EQUAL(out[0], T(0));
      v2.add(symbol("b"));
      v2.add(symbol("c"));
      v2.add(symbol("d"));
      v1.add(symbol("b"));
      v1.add(symbol("d"));
      k.push_back(T(2));
      k.push_back(T(4));
      out = k.merge_args(v1, v2);
      BOOST_CHECK_EQUAL(out.size(), unsigned(4));
      BOOST_CHECK_EQUAL(out[0], T(0));
      BOOST_CHECK_EQUAL(out[1], T(2));
      BOOST_CHECK_EQUAL(out[2], T(0));
      BOOST_CHECK_EQUAL(out[3], T(4));
      v2.add(symbol("e"));
      v2.add(symbol("f"));
      v2.add(symbol("g"));
      v2.add(symbol("h"));
      v1.add(symbol("e"));
      v1.add(symbol("g"));
      k.push_back(T(5));
      k.push_back(T(7));
      out = k.merge_args(v1, v2);
      BOOST_CHECK_EQUAL(out.size(), unsigned(8));
      BOOST_CHECK_EQUAL(out[0], T(0));
      BOOST_CHECK_EQUAL(out[1], T(2));
      BOOST_CHECK_EQUAL(out[2], T(0));
      BOOST_CHECK_EQUAL(out[3], T(4));
      BOOST_CHECK_EQUAL(out[4], T(5));
      BOOST_CHECK_EQUAL(out[5], T(0));
      BOOST_CHECK_EQUAL(out[6], T(7));
      BOOST_CHECK_EQUAL(out[7], T(0));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_merge_args_test) {
  boost::mpl::for_each<expo_types>(merge_args_tester());
}

struct is_unitary_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> key_type;
      symbol_set v1, v2;
      v2.add(symbol("a"));
      key_type k(v1);
      BOOST_CHECK(k.is_unitary(v1));
      key_type k2(v2);
      BOOST_CHECK(k2.is_unitary(v2));
      k2[0] = 1;
      BOOST_CHECK(!k2.is_unitary(v2));
      k2[0] = 0;
      BOOST_CHECK(k2.is_unitary(v2));
      BOOST_CHECK_THROW(k2.is_unitary(symbol_set{}), std::invalid_argument);
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_is_unitary_test) {
  boost::mpl::for_each<expo_types>(is_unitary_tester());
}

struct degree_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> key_type;
      key_type k0;
      symbol_set v;
      // Check the promotion of short signed ints.
      if (std::is_same<T, signed char>::value ||
          std::is_same<T, short>::value) {
        BOOST_CHECK((std::is_same<int, decltype(k0.degree(v))>::value));
      } else if (std::is_integral<T>::value) {
        BOOST_CHECK((std::is_same<T, decltype(k0.degree(v))>::value));
      }
      BOOST_CHECK(key_has_degree<key_type>::value);
      BOOST_CHECK(key_has_ldegree<key_type>::value);
      BOOST_CHECK_EQUAL(k0.degree(v), T(0));
      BOOST_CHECK_EQUAL(k0.ldegree(v), T(0));
      v.add(symbol("a"));
      key_type k1(v);
      BOOST_CHECK_EQUAL(k1.degree(v), T(0));
      BOOST_CHECK_EQUAL(k1.ldegree(v), T(0));
      k1[0] = T(2);
      BOOST_CHECK_EQUAL(k1.degree(v), T(2));
      BOOST_CHECK_EQUAL(k1.ldegree(v), T(2));
      v.add(symbol("b"));
      key_type k2(v);
      BOOST_CHECK_EQUAL(k2.degree(v), T(0));
      BOOST_CHECK_EQUAL(k2.ldegree(v), T(0));
      k2[0] = T(2);
      k2[1] = T(3);
      BOOST_CHECK_EQUAL(k2.degree(v), T(2) + T(3));
      BOOST_CHECK_THROW(k2.degree(symbol_set{}), std::invalid_argument);
      // Partial degree.
      using positions = symbol_set::positions;
      auto ss_to_pos = [](const symbol_set &vs,
                          const std::set<std::string> &s) {
        symbol_set tmp;
        for (const auto &str : s) {
          tmp.add(str);
        }
        return positions(vs, tmp);
      };
      if (std::is_same<T, signed char>::value ||
          std::is_same<T, short>::value) {
        BOOST_CHECK(
            (std::is_same<int, decltype(k2.degree(
                                   ss_to_pos(v, std::set<std::string>{}),
                                   v))>::value));
      } else if (std::is_integral<T>::value) {
        BOOST_CHECK((std::is_same<T, decltype(k2.degree(
                                         ss_to_pos(v, std::set<std::string>{}),
                                         v))>::value));
      }
      BOOST_CHECK(k2.degree(ss_to_pos(v, std::set<std::string>{}), v) == T(0));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"a"}), v) == T(2));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"A"}), v) == T(0));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"z"}), v) == T(0));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"z", "A", "a"}), v) == T(2));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"z", "A", "b"}), v) == T(3));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"B", "A", "b"}), v) == T(3));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"a", "b", "z"}), v) == T(3) + T(2));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"a", "b", "A"}), v) == T(3) + T(2));
      BOOST_CHECK(k2.degree(ss_to_pos(v, {"a", "b", "A", "z"}), v) ==
                  T(3) + T(2));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, std::set<std::string>{}), v) == T(0));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"a"}), v) == T(2));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"A"}), v) == T(0));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"z"}), v) == T(0));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"z", "A", "a"}), v) == T(2));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"z", "A", "b"}), v) == T(3));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"B", "A", "b"}), v) == T(3));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"a", "b", "z"}), v) == T(3) + T(2));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"a", "b", "A"}), v) == T(3) + T(2));
      BOOST_CHECK(k2.ldegree(ss_to_pos(v, {"a", "b", "A", "z"}), v) ==
                  T(3) + T(2));
      v.add(symbol("c"));
      key_type k3(v);
      k3[0] = T(2);
      k3[1] = T(3);
      k3[2] = T(4);
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"a", "b", "A", "z"}), v) ==
                  T(3) + T(2));
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"a", "c", "A", "z"}), v) ==
                  T(4) + T(2));
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"a", "c", "b", "z"}), v) ==
                  T(4) + T(2) + T(3));
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"a", "c", "b", "A"}), v) ==
                  T(4) + T(2) + T(3));
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"c", "b", "A"}), v) == T(4) + T(3));
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"A", "B", "C"}), v) == T(0));
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"x", "y", "z"}), v) == T(0));
      BOOST_CHECK(k3.degree(ss_to_pos(v, {"x", "y", "z", "A", "B", "C", "a"}),
                            v) == T(2));
      // Try partials with bogus positions.
      symbol_set v2({symbol("a"), symbol("b"), symbol("c"), symbol("d")});
      BOOST_CHECK_THROW(k3.degree(ss_to_pos(v2, {"d"}), v),
                        std::invalid_argument);
      BOOST_CHECK_THROW(k3.ldegree(ss_to_pos(v2, {"d"}), v),
                        std::invalid_argument);
      // Wrong symbol set, will not throw because positions are empty.
      BOOST_CHECK_EQUAL(k3.degree(ss_to_pos(v2, {"e"}), v), 0);
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_degree_test) {
  boost::mpl::for_each<expo_types>(degree_tester());
  // Test the overflowing.
  using k_type = monomial<int>;
  k_type m{std::numeric_limits<int>::max(), 1};
  symbol_set vs{symbol{"x"}, symbol{"y"}};
  BOOST_CHECK_THROW(m.degree(vs), std::overflow_error);
  m = k_type{std::numeric_limits<int>::min(), -1};
  BOOST_CHECK_THROW(m.degree(vs), std::overflow_error);
  m = k_type{std::numeric_limits<int>::min(), 1};
  BOOST_CHECK_EQUAL(m.degree(vs), std::numeric_limits<int>::min() + 1);
  // Also for partial degree.
  vs = symbol_set{symbol{"x"}, symbol{"y"}, symbol{"z"}};
  m = k_type{std::numeric_limits<int>::max(), 1, 0};
  BOOST_CHECK_EQUAL(
      m.degree(symbol_set::positions(vs, symbol_set{symbol{"x"}, symbol{"z"}}),
               vs),
      std::numeric_limits<int>::max());
  BOOST_CHECK_THROW(
      m.degree(symbol_set::positions(vs, symbol_set{symbol{"x"}, symbol{"y"}}),
               vs),
      std::overflow_error);
  m = k_type{std::numeric_limits<int>::min(), 0, -1};
  BOOST_CHECK_EQUAL(
      m.degree(symbol_set::positions(vs, symbol_set{symbol{"x"}, symbol{"y"}}),
               vs),
      std::numeric_limits<int>::min());
  BOOST_CHECK_THROW(
      m.degree(symbol_set::positions(vs, symbol_set{symbol{"x"}, symbol{"z"}}),
               vs),
      std::overflow_error);
}

// Mock cf with wrong specialisation of mul3.
struct mock_cf3 {
  mock_cf3();
  explicit mock_cf3(const int &);
  mock_cf3(const mock_cf3 &);
  mock_cf3(mock_cf3 &&) noexcept;
  mock_cf3 &operator=(const mock_cf3 &);
  mock_cf3 &operator=(mock_cf3 &&) noexcept;
  friend std::ostream &operator<<(std::ostream &, const mock_cf3 &);
  mock_cf3 operator-() const;
  bool operator==(const mock_cf3 &) const;
  bool operator!=(const mock_cf3 &) const;
  mock_cf3 &operator+=(const mock_cf3 &);
  mock_cf3 &operator-=(const mock_cf3 &);
  mock_cf3 operator+(const mock_cf3 &) const;
  mock_cf3 operator-(const mock_cf3 &) const;
  mock_cf3 &operator*=(const mock_cf3 &);
  mock_cf3 operator*(const mock_cf3 &)const;
};

namespace piranha {
namespace math {

template <typename T>
struct mul3_impl<
    T, typename std::enable_if<std::is_same<T, mock_cf3>::value>::type> {};
}
}

struct multiply_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      {
        // Integer coefficient.
        using key_type = monomial<T, U>;
        using term_type = term<integer, key_type>;
        symbol_set ed;
        ed.add("x");
        term_type t1, t2;
        t1.m_cf = 2;
        t1.m_key = key_type{T(2)};
        t2.m_cf = 3;
        t2.m_key = key_type{T(3)};
        std::array<term_type, 1u> res;
        key_type::multiply(res, t1, t2, ed);
        BOOST_CHECK_EQUAL(res[0].m_cf, t1.m_cf * t2.m_cf);
        BOOST_CHECK_EQUAL(res[0].m_key[0], T(5));
      }
      {
        // Try with rational as well, check special handling.
        using key_type = monomial<T, U>;
        using term_type = term<rational, key_type>;
        symbol_set ed;
        ed.add("x");
        ed.add("y");
        term_type t1, t2;
        t1.m_cf = 2 / 3_q;
        t1.m_key = key_type{T(2), T(-1)};
        t2.m_cf = -3;
        t2.m_key = key_type{T(3), T(7)};
        std::array<term_type, 1u> res;
        key_type::multiply(res, t1, t2, ed);
        BOOST_CHECK_EQUAL(res[0].m_cf, -6);
        BOOST_CHECK_EQUAL(res[0].m_key[0], T(5));
        BOOST_CHECK_EQUAL(res[0].m_key[1], T(6));
      }
      // Check throwing as well.
      using key_type = monomial<T, U>;
      using term_type = term<rational, key_type>;
      symbol_set ed;
      ed.add("x");
      term_type t1, t2;
      t1.m_cf = 2 / 3_q;
      t1.m_key = key_type{T(2), T(-1)};
      t2.m_cf = -3;
      t2.m_key = key_type{T(3), T(7)};
      std::array<term_type, 1u> res;
      BOOST_CHECK_THROW(key_type::multiply(res, t1, t2, ed),
                        std::invalid_argument);
      // Check the type trait.
      BOOST_CHECK((key_is_multipliable<rational, key_type>::value));
      BOOST_CHECK((key_is_multipliable<integer, key_type>::value));
      BOOST_CHECK((key_is_multipliable<double, key_type>::value));
      BOOST_CHECK((!key_is_multipliable<mock_cf3, key_type>::value));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_multiply_test) {
  boost::mpl::for_each<expo_types>(multiply_tester());
}

struct monomial_multiply_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      using key_type = monomial<T, U>;
      symbol_set ed;
      key_type k1, k2, res;
      key_type::multiply(res, k1, k2, ed);
      BOOST_CHECK(res.size() == 0u);
      ed.add("x");
      k1 = key_type{T(2)};
      k2 = key_type{T(3)};
      key_type::multiply(res, k1, k2, ed);
      BOOST_CHECK(res == key_type{T(5)});
      ed.add("y");
      BOOST_CHECK_THROW(key_type::multiply(res, k1, k2, ed),
                        std::invalid_argument);
      BOOST_CHECK(res == key_type{T(5)});
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_monomial_multiply_test) {
  boost::mpl::for_each<expo_types>(monomial_multiply_tester());
}

struct monomial_divide_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      using key_type = monomial<T, U>;
      symbol_set ed;
      key_type k1, k2, res;
      key_type::divide(res, k1, k2, ed);
      BOOST_CHECK(res.size() == 0u);
      ed.add("x");
      k1 = key_type{T(2)};
      k2 = key_type{T(3)};
      key_type::divide(res, k1, k2, ed);
      BOOST_CHECK(res == key_type{T(-1)});
      ed.add("y");
      BOOST_CHECK_THROW(key_type::divide(res, k1, k2, ed),
                        std::invalid_argument);
      BOOST_CHECK(res == key_type{T(-1)});
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_monomial_divide_test) {
  boost::mpl::for_each<expo_types>(monomial_divide_tester());
}

struct print_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      symbol_set vs;
      k_type k1;
      std::ostringstream oss;
      k1.print(oss, vs);
      BOOST_CHECK(oss.str().empty());
      vs.add("x");
      k_type k2(vs);
      k2.print(oss, vs);
      BOOST_CHECK(oss.str() == "");
      oss.str("");
      k_type k3({T(-1)});
      k3.print(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "x**-1");
      k_type k4({T(1)});
      oss.str("");
      k4.print(oss, vs);
      BOOST_CHECK(oss.str() == "x");
      k_type k5({T(-1), T(1)});
      vs.add("y");
      oss.str("");
      k5.print(oss, vs);
      BOOST_CHECK(oss.str() == "x**-1*y");
      k_type k6({T(-1), T(-2)});
      oss.str("");
      k6.print(oss, vs);
      BOOST_CHECK(oss.str() == "x**-1*y**-2");
      k_type k7;
      BOOST_CHECK_THROW(k7.print(oss, vs), std::invalid_argument);
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_print_test) {
  boost::mpl::for_each<expo_types>(print_tester());
}

struct linear_argument_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      symbol_set vs;
      BOOST_CHECK_THROW(k_type().linear_argument(vs), std::invalid_argument);
      vs.add("x");
      BOOST_CHECK_THROW(k_type().linear_argument(vs), std::invalid_argument);
      k_type k({T(1)});
      BOOST_CHECK_EQUAL(k.linear_argument(vs), "x");
      k = k_type({T(0), T(1)});
      vs.add("y");
      BOOST_CHECK_EQUAL(k.linear_argument(vs), "y");
      k = k_type({T(0), T(2)});
      BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
      k = k_type({T(2), T(0)});
      BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
      k = k_type({T(1), T(1)});
      BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_linear_argument_test) {
  boost::mpl::for_each<expo_types>(linear_argument_tester());
  typedef monomial<rational> k_type;
  symbol_set vs;
  vs.add("x");
  k_type k({rational(1, 2)});
  BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
  k = k_type({rational(1), rational(0)});
  vs.add("y");
  BOOST_CHECK_EQUAL(k.linear_argument(vs), "x");
  k = k_type({rational(2), rational(1)});
  BOOST_CHECK_THROW(k.linear_argument(vs), std::invalid_argument);
}

struct pow_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      symbol_set vs;
      k_type k1;
      BOOST_CHECK(k1 == k1.pow(42, vs));
      vs.add("x");
      BOOST_CHECK_THROW(k1.pow(42, vs), std::invalid_argument);
      k1 = k_type({T(1), T(2), T(3)});
      vs.add("y");
      vs.add("z");
      BOOST_CHECK(k1.pow(2, vs) == k_type({T(2), T(4), T(6)}));
      BOOST_CHECK(k1.pow(-2, vs) == k_type({T(-2), T(-4), T(-6)}));
      BOOST_CHECK(k1.pow(0, vs) == k_type({T(0), T(0), T(0)}));
      vs.add("a");
      BOOST_CHECK_THROW(k1.pow(42, vs), std::invalid_argument);
      T tmp;
      check_overflow(k1, tmp);
    }
  };
  template <typename KType, typename U,
            typename std::enable_if<std::is_integral<U>::value, int>::type = 0>
  static void check_overflow(const KType &, const U &) {
    KType k2({2});
    symbol_set vs2;
    vs2.add("x");
    BOOST_CHECK_THROW(k2.pow(std::numeric_limits<U>::max(), vs2),
                      std::invalid_argument);
  }
  template <typename KType, typename U,
            typename std::enable_if<!std::is_integral<U>::value, int>::type = 0>
  static void check_overflow(const KType &, const U &) {}
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_pow_test) {
  boost::mpl::for_each<expo_types>(pow_tester());
}

struct fake_int {
  fake_int();
  explicit fake_int(int);
  fake_int(const fake_int &);
  fake_int(fake_int &&) noexcept;
  fake_int &operator=(const fake_int &);
  fake_int &operator=(fake_int &&) noexcept;
  ~fake_int();
  bool operator==(const fake_int &) const;
  bool operator!=(const fake_int &) const;
  bool operator<(const fake_int &) const;
  fake_int operator+(const fake_int &) const;
  fake_int &operator+=(const fake_int &);
  // Disable the subtraction operators.
  // fake_int operator-(const fake_int &) const;
  // fake_int &operator-=(const fake_int &);
  friend std::ostream &operator<<(std::ostream &, const fake_int &);
};

struct fake_int_01 {
  fake_int_01();
  explicit fake_int_01(int);
  fake_int_01(const fake_int_01 &);
  fake_int_01(fake_int_01 &&) noexcept;
  fake_int_01 &operator=(const fake_int_01 &);
  fake_int_01 &operator=(fake_int_01 &&) noexcept;
  ~fake_int_01();
  bool operator==(const fake_int_01 &) const;
  bool operator!=(const fake_int_01 &) const;
  bool operator<(const fake_int_01 &) const;
  fake_int_01 operator+(const fake_int_01 &) const;
  fake_int_01 &operator+=(const fake_int_01 &);
  fake_int_01 operator-(const fake_int_01 &) const;
  fake_int_01 &operator-=(const fake_int_01 &);
  friend std::ostream &operator<<(std::ostream &, const fake_int_01 &);
};

namespace std {

template <> struct hash<fake_int> {
  typedef size_t result_type;
  typedef fake_int argument_type;
  result_type operator()(const argument_type &) const;
};

template <> struct hash<fake_int_01> {
  typedef size_t result_type;
  typedef fake_int_01 argument_type;
  result_type operator()(const argument_type &) const;
};
}

struct partial_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      BOOST_CHECK(key_is_differentiable<k_type>::value);
      using positions = symbol_set::positions;
      auto s_to_pos = [](const symbol_set &v, const symbol &s) {
        symbol_set tmp{s};
        return positions(v, tmp);
      };
      symbol_set vs;
      k_type k1;
      vs.add("x");
      BOOST_CHECK_THROW(k1.partial(s_to_pos(vs, symbol("x")), vs),
                        std::invalid_argument);
      k1 = k_type({T(2)});
      auto ret = k1.partial(s_to_pos(vs, symbol("x")), vs);
      BOOST_CHECK_EQUAL(ret.first, T(2));
      BOOST_CHECK(ret.second == k_type({T(1)}));
      // Derivative wrt a variable not in the monomial.
      ret = k1.partial(s_to_pos(vs, symbol("y")), vs);
      BOOST_CHECK_EQUAL(ret.first, T(0));
      BOOST_CHECK(ret.second == k_type{vs});
      // Derivative wrt a variable which has zero exponent.
      k1 = k_type({T(0)});
      ret = k1.partial(s_to_pos(vs, symbol("x")), vs);
      BOOST_CHECK_EQUAL(ret.first, T(0));
      BOOST_CHECK(ret.second == k_type{vs});
      vs.add("y");
      k1 = k_type({T(-1), T(0)});
      ret = k1.partial(s_to_pos(vs, symbol("y")), vs);
      // y has zero exponent.
      BOOST_CHECK_EQUAL(ret.first, T(0));
      BOOST_CHECK(ret.second == k_type{vs});
      ret = k1.partial(s_to_pos(vs, symbol("x")), vs);
      BOOST_CHECK_EQUAL(ret.first, T(-1));
      BOOST_CHECK(ret.second == k_type({T(-2), T(0)}));
      // Check with bogus positions.
      symbol_set vs2;
      vs2.add("x");
      vs2.add("y");
      vs2.add("z");
      // The z variable is in position 2, which is outside the size of the
      // monomial.
      BOOST_CHECK_THROW(k1.partial(s_to_pos(vs2, symbol("z")), vs),
                        std::invalid_argument);
      // Derivative wrt multiple variables.
      BOOST_CHECK_THROW(
          k1.partial(symbol_set::positions(
                         vs2, symbol_set({symbol("x"), symbol("y")})),
                     vs),
          std::invalid_argument);
      // Check the overflow check.
      overflow_check(k1);
    }
    template <
        typename T2, typename U2,
        typename std::enable_if<std::is_integral<T2>::value, int>::type = 0>
    static void overflow_check(const monomial<T2, U2> &) {
      using positions = symbol_set::positions;
      auto s_to_pos = [](const symbol_set &v, const symbol &s) {
        symbol_set tmp{s};
        return positions(v, tmp);
      };
      monomial<T2, U2> k({std::numeric_limits<T2>::min()});
      symbol_set vs;
      vs.add("x");
      BOOST_CHECK_THROW(k.partial(s_to_pos(vs, symbol("x")), vs),
                        std::invalid_argument);
    }
    template <
        typename T2, typename U2,
        typename std::enable_if<!std::is_integral<T2>::value, int>::type = 0>
    static void overflow_check(const monomial<T2, U2> &) {}
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
    // fake_int has no subtraction operators.
    BOOST_CHECK((!key_is_differentiable<monomial<fake_int>>::value));
    BOOST_CHECK((key_is_differentiable<monomial<fake_int_01>>::value));
  }
};

BOOST_AUTO_TEST_CASE(monomial_partial_test) {
  boost::mpl::for_each<expo_types>(partial_tester());
}

struct evaluate_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      using k_type = monomial<T, U>;
      using pmap_type1 = symbol_set::positions_map<integer>;
      using dict_type1 = std::unordered_map<symbol, integer>;
      BOOST_CHECK((key_is_evaluable<k_type, integer>::value));
      symbol_set vs;
      k_type k1;
      BOOST_CHECK_EQUAL(k1.evaluate(pmap_type1(vs, dict_type1{}), vs),
                        integer(1));
      vs.add("x");
      // Mismatch between the size of k1 and vs.
      BOOST_CHECK_THROW(k1.evaluate(pmap_type1(vs, dict_type1{}), vs),
                        std::invalid_argument);
      k1 = k_type({T(1)});
      // Empty pmap, k1 has non-zero size.
      BOOST_CHECK_THROW(k1.evaluate(pmap_type1(vs, dict_type1{}), vs),
                        std::invalid_argument);
      BOOST_CHECK_EQUAL(
          k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(1)}}),
                      vs),
          1);
      // pmap with invalid position, 1, where the monomial has only 1 element.
      BOOST_CHECK_THROW(
          k1.evaluate(pmap_type1(symbol_set{symbol{"a"}, symbol{"b"}},
                                 dict_type1{{symbol{"b"}, integer(4)}}),
                      vs),
          std::invalid_argument);
      k1 = k_type({T(2)});
      BOOST_CHECK_EQUAL(
          k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(3)}}),
                      vs),
          9);
      BOOST_CHECK_EQUAL(
          k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(3)},
                                                {symbol("y"), integer(4)}}),
                      vs),
          9);
      k1 = k_type({T(2), T(4)});
      vs.add("y");
      BOOST_CHECK_EQUAL(
          k1.evaluate(pmap_type1(vs, dict_type1{{symbol("x"), integer(3)},
                                                {symbol("y"), integer(4)}}),
                      vs),
          2304);
      BOOST_CHECK_EQUAL(
          k1.evaluate(pmap_type1(vs, dict_type1{{symbol("y"), integer(4)},
                                                {symbol("x"), integer(3)}}),
                      vs),
          2304);
      // pmap has correctly 2 elements, but they refer to indices 0 and 2.
      BOOST_CHECK_THROW(
          k1.evaluate(
              pmap_type1(symbol_set{symbol{"a"}, symbol{"b"}, symbol{"c"}},
                         dict_type1{{symbol{"a"}, integer(4)},
                                    {symbol{"c"}, integer(4)}}),
              vs),
          std::invalid_argument);
      // Same with indices 1 and 2.
      BOOST_CHECK_THROW(
          k1.evaluate(
              pmap_type1(symbol_set{symbol{"a"}, symbol{"b"}, symbol{"c"}},
                         dict_type1{{symbol{"b"}, integer(4)},
                                    {symbol{"c"}, integer(4)}}),
              vs),
          std::invalid_argument);
      using pmap_type2 = symbol_set::positions_map<double>;
      using dict_type2 = std::unordered_map<symbol, double>;
      BOOST_CHECK_EQUAL(
          k1.evaluate(pmap_type2(vs, dict_type2{{symbol("y"), -4.3},
                                                {symbol("x"), 3.2}}),
                      vs),
          math::pow(3.2, 2) * math::pow(-4.3, 4));
      using pmap_type3 = symbol_set::positions_map<rational>;
      using dict_type3 = std::unordered_map<symbol, rational>;
      BOOST_CHECK_EQUAL(
          k1.evaluate(
              pmap_type3(vs, dict_type3{{symbol("y"), rational(1, 2)},
                                        {symbol("x"), rational(-4, 3)}}),
              vs),
          math::pow(rational(4, -3), 2) * math::pow(rational(-1, -2), 4));
      k1 = k_type({T(-2), T(-4)});
      BOOST_CHECK_EQUAL(
          k1.evaluate(
              pmap_type3(vs, dict_type3{{symbol("y"), rational(1, 2)},
                                        {symbol("x"), rational(-4, 3)}}),
              vs),
          math::pow(rational(4, -3), -2) * math::pow(rational(-1, -2), -4));
      using pmap_type4 = symbol_set::positions_map<real>;
      using dict_type4 = std::unordered_map<symbol, real>;
      BOOST_CHECK_EQUAL(
          k1.evaluate(pmap_type4(vs, dict_type4{{symbol("y"), real(1.234)},
                                                {symbol("x"), real(5.678)}}),
                      vs),
          math::pow(real(5.678), -2) * math::pow(real(1.234), -4));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_evaluate_test) {
  boost::mpl::for_each<expo_types>(evaluate_tester());
  BOOST_CHECK((key_is_evaluable<monomial<rational>, double>::value));
  BOOST_CHECK((key_is_evaluable<monomial<rational>, real>::value));
  BOOST_CHECK((!key_is_evaluable<monomial<rational>, std::string>::value));
  BOOST_CHECK((!key_is_evaluable<monomial<rational>, void *>::value));
}

struct subs_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      symbol_set vs;
      k_type k1;
      // Test the type trait.
      BOOST_CHECK((key_has_subs<k_type, integer>::value));
      BOOST_CHECK((key_has_subs<k_type, rational>::value));
      BOOST_CHECK((key_has_subs<k_type, real>::value));
      BOOST_CHECK((key_has_subs<k_type, double>::value));
      BOOST_CHECK((!key_has_subs<k_type, std::string>::value));
      BOOST_CHECK((!key_has_subs<k_type, std::vector<std::string>>::value));
      auto ret = k1.subs("x", integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK((std::is_same<integer, decltype(ret[0u].first)>::value));
      BOOST_CHECK(ret[0u].second == k1);
      vs.add("x");
      BOOST_CHECK_THROW(k1.subs("x", integer(4), vs), std::invalid_argument);
      k1 = k_type({T(2)});
      ret = k1.subs("y", integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK(ret[0u].second == k1);
      ret = k1.subs("x", integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(4), T(2)));
      BOOST_CHECK(ret[0u].second == k_type({T(0)}));
      k1 = k_type({T(2), T(3)});
      BOOST_CHECK_THROW(k1.subs("x", integer(4), vs), std::invalid_argument);
      vs.add("y");
      ret = k1.subs("y", integer(-2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(-2), T(3)));
      BOOST_CHECK((ret[0u].second == k_type{T(2), T(0)}));
      auto ret2 = k1.subs("x", real(-2.345), vs);
      BOOST_CHECK_EQUAL(ret2.size(), 1u);
      BOOST_CHECK_EQUAL(ret2[0u].first, math::pow(real(-2.345), T(2)));
      BOOST_CHECK((ret2[0u].second == k_type{T(0), T(3)}));
      BOOST_CHECK((std::is_same<real, decltype(ret2[0u].first)>::value));
      auto ret3 = k1.subs("x", rational(-1, 2), vs);
      BOOST_CHECK_EQUAL(ret3.size(), 1u);
      BOOST_CHECK_EQUAL(ret3[0u].first, rational(1, 4));
      BOOST_CHECK((ret3[0u].second == k_type{T(0), T(3)}));
      BOOST_CHECK((std::is_same<rational, decltype(ret3[0u].first)>::value));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_subs_test) {
  boost::mpl::for_each<expo_types>(subs_tester());
}

struct print_tex_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      symbol_set vs;
      k_type k1;
      std::ostringstream oss;
      k1.print_tex(oss, vs);
      BOOST_CHECK(oss.str().empty());
      k1 = k_type({T(0)});
      BOOST_CHECK_THROW(k1.print_tex(oss, vs), std::invalid_argument);
      vs.add("x");
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "");
      k1 = k_type({T(1)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "{x}");
      oss.str("");
      k1 = k_type({T(-1)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{x}}");
      oss.str("");
      k1 = k_type({T(2)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "{x}^{2}");
      oss.str("");
      k1 = k_type({T(-2)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{x}^{2}}");
      vs.add("y");
      oss.str("");
      k1 = k_type({T(-2), T(1)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "\\frac{{y}}{{x}^{2}}");
      oss.str("");
      k1 = k_type({T(-2), T(3)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "\\frac{{y}^{3}}{{x}^{2}}");
      oss.str("");
      k1 = k_type({T(-2), T(-3)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{x}^{2}{y}^{3}}");
      oss.str("");
      k1 = k_type({T(2), T(3)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "{x}^{2}{y}^{3}");
      oss.str("");
      k1 = k_type({T(1), T(3)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "{x}{y}^{3}");
      oss.str("");
      k1 = k_type({T(0), T(3)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "{y}^{3}");
      oss.str("");
      k1 = k_type({T(0), T(0)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "");
      oss.str("");
      k1 = k_type({T(0), T(1)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "{y}");
      oss.str("");
      k1 = k_type({T(0), T(-1)});
      k1.print_tex(oss, vs);
      BOOST_CHECK_EQUAL(oss.str(), "\\frac{1}{{y}}");
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_print_tex_test) {
  boost::mpl::for_each<expo_types>(print_tex_tester());
}

struct integrate_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      BOOST_CHECK(key_is_integrable<k_type>::value);
      symbol_set vs;
      k_type k1;
      auto ret = k1.integrate(symbol("a"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(1));
      BOOST_CHECK(ret.second == k_type({T(1)}));
      vs.add("b");
      BOOST_CHECK_THROW(k1.integrate(symbol("b"), vs), std::invalid_argument);
      k1 = k_type{T(1)};
      ret = k1.integrate(symbol("b"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(2));
      BOOST_CHECK(ret.second == k_type({T(2)}));
      k1 = k_type{T(2)};
      ret = k1.integrate(symbol("c"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(1));
      BOOST_CHECK(ret.second == k_type({T(2), T(1)}));
      ret = k1.integrate(symbol("a"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(1));
      BOOST_CHECK(ret.second == k_type({T(1), T(2)}));
      k1 = k_type{T(2), T(3)};
      vs.add("d");
      ret = k1.integrate(symbol("a"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(1));
      BOOST_CHECK(ret.second == k_type({T(1), T(2), T(3)}));
      ret = k1.integrate(symbol("b"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(3));
      BOOST_CHECK(ret.second == k_type({T(3), T(3)}));
      ret = k1.integrate(symbol("c"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(1));
      BOOST_CHECK(ret.second == k_type({T(2), T(1), T(3)}));
      ret = k1.integrate(symbol("d"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(4));
      BOOST_CHECK(ret.second == k_type({T(2), T(4)}));
      ret = k1.integrate(symbol("e"), vs);
      BOOST_CHECK_EQUAL(ret.first, T(1));
      BOOST_CHECK(ret.second == k_type({T(2), T(3), T(1)}));
      k1 = k_type{T(-1), T(3)};
      BOOST_CHECK_THROW(k1.integrate(symbol("b"), vs), std::invalid_argument);
      k1 = k_type{T(2), T(-1)};
      BOOST_CHECK_THROW(k1.integrate(symbol("d"), vs), std::invalid_argument);
      // Overflow check.
      overflow_check(k1);
    }
  };
  template <typename U,
            typename std::enable_if<
                std::is_integral<typename U::value_type>::value, int>::type = 0>
  static void overflow_check(const U &) {
    using k_type = U;
    using T = typename k_type::value_type;
    symbol_set vs;
    vs.add("a");
    vs.add("b");
    k_type k1{T(1), std::numeric_limits<T>::max()};
    auto ret = k1.integrate(symbol("a"), vs);
    BOOST_CHECK_EQUAL(ret.first, T(2));
    BOOST_CHECK((ret.second == k_type{T(2), std::numeric_limits<T>::max()}));
    BOOST_CHECK_THROW(k1.integrate(symbol("b"), vs), std::invalid_argument);
  }
  template <typename U, typename std::enable_if<
                            !std::is_integral<typename U::value_type>::value,
                            int>::type = 0>
  static void overflow_check(const U &) {}
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_integrate_test) {
  boost::mpl::for_each<expo_types>(integrate_tester());
}

struct ipow_subs_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      // Test the type trait.
      BOOST_CHECK((key_has_ipow_subs<k_type, integer>::value));
      BOOST_CHECK((key_has_ipow_subs<k_type, double>::value));
      BOOST_CHECK((key_has_ipow_subs<k_type, real>::value));
      BOOST_CHECK((key_has_ipow_subs<k_type, rational>::value));
      BOOST_CHECK((!key_has_ipow_subs<k_type, std::string>::value));
      symbol_set vs;
      k_type k1;
      auto ret = k1.ipow_subs("x", integer(45), integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK((std::is_same<integer, decltype(ret[0u].first)>::value));
      BOOST_CHECK(ret[0u].second == k1);
      vs.add("x");
      BOOST_CHECK_THROW(k1.ipow_subs("x", integer(35), integer(4), vs),
                        std::invalid_argument);
      k1 = k_type({T(2)});
      ret = k1.ipow_subs("y", integer(2), integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK(ret[0u].second == k1);
      ret = k1.ipow_subs("x", integer(1), integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(4), T(2)));
      BOOST_CHECK(ret[0u].second == k_type({T(0)}));
      ret = k1.ipow_subs("x", integer(2), integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(4), T(1)));
      BOOST_CHECK(ret[0u].second == k_type({T(0)}));
      ret = k1.ipow_subs("x", integer(-1), integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK(ret[0u].second == k_type({T(2)}));
      ret = k1.ipow_subs("x", integer(4), integer(4), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK(ret[0u].second == k_type({T(2)}));
      k1 = k_type({T(7), T(2)});
      BOOST_CHECK_THROW(k1.ipow_subs("x", integer(4), integer(4), vs),
                        std::invalid_argument);
      vs.add("y");
      ret = k1.ipow_subs("x", integer(3), integer(2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(2)));
      BOOST_CHECK((ret[0u].second == k_type{T(1), T(2)}));
      ret = k1.ipow_subs("x", integer(4), integer(2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(1)));
      BOOST_CHECK((ret[0u].second == k_type{T(3), T(2)}));
      ret = k1.ipow_subs("x", integer(-4), integer(2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK((ret[0u].second == k_type{T(7), T(2)}));
      k1 = k_type({T(-7), T(2)});
      ret = k1.ipow_subs("x", integer(4), integer(2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, 1);
      BOOST_CHECK((ret[0u].second == k_type{T(-7), T(2)}));
      ret = k1.ipow_subs("x", integer(-4), integer(2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(1)));
      BOOST_CHECK((ret[0u].second == k_type{T(-3), T(2)}));
      ret = k1.ipow_subs("x", integer(-3), integer(2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(2)));
      BOOST_CHECK((ret[0u].second == k_type{T(-1), T(2)}));
      k1 = k_type({T(2), T(-7)});
      ret = k1.ipow_subs("y", integer(-3), integer(2), vs);
      BOOST_CHECK_EQUAL(ret.size(), 1u);
      BOOST_CHECK_EQUAL(ret[0u].first, math::pow(integer(2), T(2)));
      BOOST_CHECK((ret[0u].second == k_type{T(2), T(-1)}));
      BOOST_CHECK_THROW(k1.ipow_subs("y", integer(0), integer(2), vs),
                        zero_division_error);
      k1 = k_type({T(-7), T(2)});
      auto ret2 = k1.ipow_subs("x", integer(-4), real(-2.345), vs);
      BOOST_CHECK_EQUAL(ret2.size(), 1u);
      BOOST_CHECK_EQUAL(ret2[0u].first, math::pow(real(-2.345), T(1)));
      BOOST_CHECK((ret2[0u].second == k_type{T(-3), T(2)}));
      BOOST_CHECK((std::is_same<real, decltype(ret2[0u].first)>::value));
      auto ret3 = k1.ipow_subs("x", integer(-3), rational(-1, 2), vs);
      BOOST_CHECK_EQUAL(ret3.size(), 1u);
      BOOST_CHECK_EQUAL(ret3[0u].first, math::pow(rational(-1, 2), T(2)));
      BOOST_CHECK((ret3[0u].second == k_type{T(-1), T(2)}));
      BOOST_CHECK((std::is_same<rational, decltype(ret3[0u].first)>::value));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_ipow_subs_test) {
  boost::mpl::for_each<expo_types>(ipow_subs_tester());
}

struct tt_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      typedef monomial<T, U> k_type;
      BOOST_CHECK((!key_has_t_subs<k_type, int, int>::value));
      BOOST_CHECK((!key_has_t_subs<k_type &, int, int>::value));
      BOOST_CHECK((!key_has_t_subs<k_type &&, int, int>::value));
      BOOST_CHECK((!key_has_t_subs<const k_type &, int, int>::value));
      BOOST_CHECK(is_container_element<k_type>::value);
      BOOST_CHECK(is_hashable<k_type>::value);
      BOOST_CHECK(key_has_degree<k_type>::value);
      BOOST_CHECK(key_has_ldegree<k_type>::value);
      BOOST_CHECK(!key_has_t_degree<k_type>::value);
      BOOST_CHECK(!key_has_t_ldegree<k_type>::value);
      BOOST_CHECK(!key_has_t_order<k_type>::value);
      BOOST_CHECK(!key_has_t_lorder<k_type>::value);
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_type_traits_test) {
  boost::mpl::for_each<expo_types>(tt_tester());
}

struct serialization_tester {
  template <typename T> void operator()(const T &) {
    typedef monomial<int, T> k_type;
    k_type tmp;
    std::stringstream ss;
    k_type k0({1, 2, 3, 4, 5});
    {
      boost::archive::text_oarchive oa(ss);
      oa << k0;
    }
    {
      boost::archive::text_iarchive ia(ss);
      ia >> tmp;
    }
    BOOST_CHECK(tmp == k0);
    ss.str("");
    k_type k1({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    {
      boost::archive::text_oarchive oa(ss);
      oa << k1;
    }
    {
      boost::archive::text_iarchive ia(ss);
      ia >> tmp;
    }
    BOOST_CHECK(tmp == k1);
  }
};

BOOST_AUTO_TEST_CASE(monomial_serialization_test) {
  boost::mpl::for_each<size_types>(serialization_tester());
}

BOOST_AUTO_TEST_CASE(monomial_kic_test) {
  using k_type_00 = monomial<int>;
  using k_type_01 = monomial<long>;
  using k_type_02 = monomial<long>;
  BOOST_CHECK((key_is_convertible<k_type_00, k_type_00>::value));
  BOOST_CHECK((key_is_convertible<k_type_01, k_type_01>::value));
  BOOST_CHECK((key_is_convertible<k_type_02, k_type_02>::value));
  BOOST_CHECK((key_is_convertible<k_type_00, k_type_01>::value));
  BOOST_CHECK((key_is_convertible<k_type_01, k_type_00>::value));
  BOOST_CHECK((key_is_convertible<k_type_00, k_type_02>::value));
  BOOST_CHECK((key_is_convertible<k_type_02, k_type_00>::value));
  BOOST_CHECK((key_is_convertible<k_type_01, k_type_02>::value));
  BOOST_CHECK((key_is_convertible<k_type_02, k_type_01>::value));
  BOOST_CHECK((!key_is_convertible<k_type_00, k_monomial>::value));
  BOOST_CHECK((!key_is_convertible<k_monomial, k_type_00>::value));
}

BOOST_AUTO_TEST_CASE(monomial_comparison_test) {
  using k_type_00 = monomial<int>;
  BOOST_CHECK(is_less_than_comparable<k_type_00>::value);
  BOOST_CHECK(!(k_type_00{} < k_type_00{}));
  BOOST_CHECK(!(k_type_00{3} < k_type_00{2}));
  BOOST_CHECK(!(k_type_00{3} < k_type_00{3}));
  BOOST_CHECK(k_type_00{2} < k_type_00{3});
  BOOST_CHECK((k_type_00{2, 3} < k_type_00{2, 4}));
  BOOST_CHECK(!(k_type_00{2, 2} < k_type_00{2, 2}));
  BOOST_CHECK((k_type_00{1, 3} < k_type_00{2, 1}));
  BOOST_CHECK(!(k_type_00{1, 2, 3, 4} < k_type_00{1, 2, 3, 4}));
  BOOST_CHECK_THROW((void)(k_type_00{} < k_type_00{1}), std::invalid_argument);
  BOOST_CHECK_THROW((void)(k_type_00{1} < k_type_00{}), std::invalid_argument);
}

struct split_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      using k_type = monomial<T, U>;
      symbol_set vs;
      BOOST_CHECK_THROW(k_type{}.split(vs), std::invalid_argument);
      vs.add("x");
      BOOST_CHECK_THROW(k_type{}.split(vs), std::invalid_argument);
      BOOST_CHECK_THROW(k_type{T(1)}.split(vs), std::invalid_argument);
      vs.add("y");
      auto res = k_type{T(1), T(2)}.split(vs);
      BOOST_CHECK_EQUAL(res.first.size(), 1u);
      BOOST_CHECK_EQUAL(res.first[0u], T(2));
      BOOST_CHECK_EQUAL(res.second.size(), 1u);
      BOOST_CHECK_EQUAL(res.second[0u], T(1));
      vs.add("z");
      BOOST_CHECK_THROW((k_type{T(1), T(2)}.split(vs)), std::invalid_argument);
      res = k_type{T(1), T(2), T(3)}.split(vs);
      BOOST_CHECK_EQUAL(res.first.size(), 2u);
      BOOST_CHECK_EQUAL(res.first[0u], T(2));
      BOOST_CHECK_EQUAL(res.first[1u], T(3));
      BOOST_CHECK_EQUAL(res.second.size(), 1u);
      BOOST_CHECK_EQUAL(res.second[0u], T(1));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_split_test) {
  boost::mpl::for_each<expo_types>(split_tester());
}

struct extract_exponents_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      using key_type = monomial<T, U>;
      std::vector<T> out;
      key_type k{};
      symbol_set ss;
      k.extract_exponents(out, ss);
      BOOST_CHECK_EQUAL(out.size(), 0u);
      ss.add(symbol{"a"});
      BOOST_CHECK_THROW(k.extract_exponents(out, ss), std::invalid_argument);
      BOOST_CHECK_EQUAL(out.size(), 0u);
      k = key_type{T(-2)};
      k.extract_exponents(out, ss);
      BOOST_CHECK_EQUAL(out.size(), 1u);
      BOOST_CHECK_EQUAL(out[0u], T(-2));
      ss.add(symbol{"b"});
      BOOST_CHECK_THROW(k.extract_exponents(out, ss), std::invalid_argument);
      BOOST_CHECK_EQUAL(out.size(), 1u);
      BOOST_CHECK_EQUAL(out[0u], T(-2));
      k = key_type{T(-2), T(3)};
      out.resize(4u);
      k.extract_exponents(out, ss);
      BOOST_CHECK_EQUAL(out.size(), 2u);
      BOOST_CHECK_EQUAL(out[0u], T(-2));
      BOOST_CHECK_EQUAL(out[1u], T(3));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_extract_exponents_test) {
  boost::mpl::for_each<expo_types>(extract_exponents_tester());
}

struct has_negative_exponent_tester {
  template <typename T> struct runner {
    template <typename U> void operator()(const U &) {
      using key_type = monomial<T, U>;
      key_type k{};
      symbol_set ss;
      BOOST_CHECK(!k.has_negative_exponent(ss));
      ss.add("x");
      BOOST_CHECK_THROW(k.has_negative_exponent(ss), std::invalid_argument);
      k = key_type{T(1)};
      BOOST_CHECK(!k.has_negative_exponent(ss));
      k = key_type{T(0)};
      BOOST_CHECK(!k.has_negative_exponent(ss));
      k = key_type{T(-1)};
      BOOST_CHECK(k.has_negative_exponent(ss));
      ss.add("y");
      BOOST_CHECK_THROW(k.has_negative_exponent(ss), std::invalid_argument);
      k = key_type{T(0), T(1)};
      BOOST_CHECK(!k.has_negative_exponent(ss));
      k = key_type{T(0), T(-1)};
      BOOST_CHECK(k.has_negative_exponent(ss));
    }
  };
  template <typename T> void operator()(const T &) {
    boost::mpl::for_each<size_types>(runner<T>());
  }
};

BOOST_AUTO_TEST_CASE(monomial_has_negative_exponent_test) {
  boost::mpl::for_each<expo_types>(has_negative_exponent_tester());
}
