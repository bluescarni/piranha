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

#include "../src/array_key.hpp"

#define BOOST_TEST_MODULE array_key_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

#include "../src/debug_access.hpp"
#include "../src/environment.hpp"
#include "../src/mp_integer.hpp"
#include "../src/symbol_set.hpp"
#include "../src/symbol.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<char,unsigned,integer> value_types;
typedef boost::mpl::vector<std::integral_constant<std::size_t,0u>,std::integral_constant<std::size_t,1u>,std::integral_constant<std::size_t,5u>,
	std::integral_constant<std::size_t,10u>> size_types;

template <typename T, typename ... Args>
class g_key_type: public array_key<T,g_key_type<T,Args...>,Args...>
{
		using base = array_key<T,g_key_type<T,Args...>,Args...>;
	public:
		g_key_type() = default;
		g_key_type(const g_key_type &) = default;
		g_key_type(g_key_type &&) = default;
		g_key_type &operator=(const g_key_type &) = default;
		g_key_type &operator=(g_key_type &&) = default;
		template <typename U>
		explicit g_key_type(std::initializer_list<U> list):base(list) {}
		template <typename ... Args2>
		explicit g_key_type(Args2 && ... params):base(std::forward<Args2>(params)...) {}
};

namespace std
{

template <typename T, typename ... Args>
struct hash<g_key_type<T,Args...>>: public hash<array_key<T,g_key_type<T,Args...>,Args...>> {};

}

// Constructors, assignments and element access.
struct constructor_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			key_type k0;
			BOOST_CHECK_NO_THROW(key_type tmp = key_type());
			BOOST_CHECK_NO_THROW(key_type tmp = key_type(key_type()));
			BOOST_CHECK_NO_THROW(key_type tmp(k0));
			// From init list.
			key_type k1{T(0),T(1),T(2),T(3)};
			BOOST_CHECK_EQUAL(k1.size(),static_cast<decltype(k1.size())>(4));
			for (typename key_type::size_type i = 0; i < k1.size(); ++i) {
				BOOST_CHECK_EQUAL(k1[i],i);
				BOOST_CHECK_NO_THROW(k1[i] = static_cast<T>(T(i) + T(1)));
				BOOST_CHECK_EQUAL(k1[i],T(i) + T(1));
			}
			key_type k1a{0,1,2,3};
			BOOST_CHECK_EQUAL(k1a.size(),static_cast<decltype(k1a.size())>(4));
			for (typename key_type::size_type i = 0; i < k1a.size(); ++i) {
				BOOST_CHECK_EQUAL(k1a[i],i);
				BOOST_CHECK_NO_THROW(k1a[i] = static_cast<T>(T(i) + T(1)));
				BOOST_CHECK_EQUAL(k1a[i],T(i) + T(1));
			}
			BOOST_CHECK_NO_THROW(k0 = k1);
			BOOST_CHECK_NO_THROW(k0 = std::move(k1));
			// Constructor from vector of symbols.
			symbol_set vs({symbol("a"),symbol("b"),symbol("c")});
			key_type k2(vs);
			BOOST_CHECK_EQUAL(k2.size(),vs.size());
			BOOST_CHECK_EQUAL(k2[0],T(0));
			BOOST_CHECK_EQUAL(k2[1],T(0));
			BOOST_CHECK_EQUAL(k2[2],T(0));
			// Generic constructor for use in series.
			BOOST_CHECK_THROW(key_type tmp(k2,symbol_set{}),std::invalid_argument);
			key_type k3(k2,vs);
			BOOST_CHECK_EQUAL(k3.size(),vs.size());
			BOOST_CHECK_EQUAL(k3[0],T(0));
			BOOST_CHECK_EQUAL(k3[1],T(0));
			BOOST_CHECK_EQUAL(k3[2],T(0));
			key_type k4(key_type(vs),vs);
			BOOST_CHECK_EQUAL(k4.size(),vs.size());
			BOOST_CHECK_EQUAL(k4[0],T(0));
			BOOST_CHECK_EQUAL(k4[1],T(0));
			BOOST_CHECK_EQUAL(k4[2],T(0));
			typedef g_key_type<int,U> key_type2;
			key_type2 k5(vs);
			BOOST_CHECK_THROW(key_type tmp(k5,symbol_set{}),std::invalid_argument);
			key_type k6(k5,vs);
			BOOST_CHECK_EQUAL(k6.size(),vs.size());
			BOOST_CHECK_EQUAL(k6[0],T(0));
			BOOST_CHECK_EQUAL(k6[1],T(0));
			BOOST_CHECK_EQUAL(k6[2],T(0));
			key_type k7(key_type2(vs),vs);
			BOOST_CHECK_EQUAL(k7.size(),vs.size());
			BOOST_CHECK_EQUAL(k7[0],T(0));
			BOOST_CHECK_EQUAL(k7[1],T(0));
			BOOST_CHECK_EQUAL(k7[2],T(0));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_constructor_test)
{
	environment env;
	boost::mpl::for_each<value_types>(constructor_tester());
}

struct hash_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			key_type k0;
			BOOST_CHECK_EQUAL(k0.hash(),std::size_t());
			BOOST_CHECK_EQUAL(k0.hash(),std::hash<key_type>()(k0));
			key_type k1{T(0),T(1),T(2),T(3)};
			BOOST_CHECK_EQUAL(k1.hash(),std::hash<key_type>()(k1));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_hash_test)
{
	boost::mpl::for_each<value_types>(hash_tester());
}

struct push_back_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			using size_type = typename key_type::size_type;
			key_type k0;
			for (size_type i = 0u; i < 4u; ++i) {
				k0.push_back(T(i));
				BOOST_CHECK_EQUAL(k0[i],T(i));
			}
			key_type k1;
			for (size_type i = 0u; i < 4u; ++i) {
				T tmp = static_cast<T>(i);
				k1.push_back(tmp);
				BOOST_CHECK_EQUAL(k1[i],tmp);
			}
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_push_back_test)
{
	boost::mpl::for_each<value_types>(push_back_tester());
}

struct equality_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			key_type k0;
			BOOST_CHECK(k0 == key_type());
			for (int i = 0; i < 4; ++i) {
				k0.push_back(T(i));
			}
			key_type k1{T(0),T(1),T(2),T(3)};
			BOOST_CHECK(k0 == k1);
			// Inequality.
			k0 = key_type();
			BOOST_CHECK(k0 != k1);
			for (int i = 0; i < 3; ++i) {
				k0.push_back(T(i));
			}
			BOOST_CHECK(k0 != k1);
			k0.push_back(T(3));
			k0.push_back(T());
			BOOST_CHECK(k0 != k1);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_equality_test)
{
	boost::mpl::for_each<value_types>(equality_tester());
}

struct merge_args_tag;

namespace piranha
{

template <>
class debug_access<merge_args_tag>
{
	public:
		template <typename T>
		struct runner
		{
			template <typename U>
			void operator()(const U &)
			{
				typedef g_key_type<T,U> key_type;
				symbol_set v1, v2;
				v2.add(symbol("a"));
				key_type k;
				auto out = k.base_merge_args(v1,v2);
				BOOST_CHECK_EQUAL(out.size(),unsigned(1));
				BOOST_CHECK_EQUAL(out[0],T(0));
				v2.add(symbol("b"));
				v2.add(symbol("c"));
				v2.add(symbol("d"));
				v1.add(symbol("b"));
				v1.add(symbol("d"));
				k.push_back(T(2));
				k.push_back(T(4));
				out = k.base_merge_args(v1,v2);
				BOOST_CHECK_EQUAL(out.size(),unsigned(4));
				BOOST_CHECK_EQUAL(out[0],T(0));
				BOOST_CHECK_EQUAL(out[1],T(2));
				BOOST_CHECK_EQUAL(out[2],T(0));
				BOOST_CHECK_EQUAL(out[3],T(4));
				v2.add(symbol("e"));
				v2.add(symbol("f"));
				v2.add(symbol("g"));
				v2.add(symbol("h"));
				v1.add(symbol("e"));
				v1.add(symbol("g"));
				k.push_back(T(5));
				k.push_back(T(7));
				out = k.base_merge_args(v1,v2);
				BOOST_CHECK_EQUAL(out.size(),unsigned(8));
				BOOST_CHECK_EQUAL(out[0],T(0));
				BOOST_CHECK_EQUAL(out[1],T(2));
				BOOST_CHECK_EQUAL(out[2],T(0));
				BOOST_CHECK_EQUAL(out[3],T(4));
				BOOST_CHECK_EQUAL(out[4],T(5));
				BOOST_CHECK_EQUAL(out[5],T(0));
				BOOST_CHECK_EQUAL(out[6],T(7));
				BOOST_CHECK_EQUAL(out[7],T(0));
				BOOST_CHECK_THROW(k.base_merge_args(v2,v1),std::invalid_argument);
				BOOST_CHECK_THROW(k.base_merge_args(v1,symbol_set{}),std::invalid_argument);
				v1.add(symbol("z"));
				BOOST_CHECK_THROW(k.base_merge_args(v1,v2),std::invalid_argument);
			}
		};
		template <typename T>
		void operator()(const T &)
		{
			boost::mpl::for_each<size_types>(runner<T>());
		}
};

}

typedef piranha::debug_access<merge_args_tag> merge_args_tester;

BOOST_AUTO_TEST_CASE(array_key_merge_args_test)
{
	boost::mpl::for_each<value_types>(merge_args_tester());
}

struct iterators_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			key_type k0;
			BOOST_CHECK(k0.begin() == k0.end());
			for (int i = 0; i < 4; ++i) {
				k0.push_back(T(i));
			}
			BOOST_CHECK(k0.begin() + 4 == k0.end());
			BOOST_CHECK(k0.begin() != k0.end());
			const key_type k1 = key_type();
			BOOST_CHECK(k1.begin() == k1.end());
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_iterators_test)
{
	boost::mpl::for_each<value_types>(iterators_tester());
}

struct resize_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			key_type k0;
			BOOST_CHECK(k0.size() == 0u);
			k0.resize(1u);
			BOOST_CHECK(k0.size() == 1u);
			k0.resize(10u);
			BOOST_CHECK(k0.size() == 10u);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_resize_test)
{
	boost::mpl::for_each<value_types>(resize_tester());
}

struct add_tag;

namespace piranha
{

// NOTE: the debug access is not actually needed any more.
template <>
class debug_access<add_tag>
{
	public:
		template <typename T>
		struct runner
		{
			template <typename U>
			void operator()(const U &)
			{
				typedef g_key_type<T,U> key_type;
				key_type k1, k2, retval;
				k1.vector_add(retval,k2);
				BOOST_CHECK(!retval.size());
				k1.resize(1);
				k2.resize(1);
				k1[0] = 1;
				k2[0] = 2;
				k1.vector_add(retval,k2);
				BOOST_CHECK(retval.size() == 1u);
				BOOST_CHECK(retval[0] == T(3));
			}
		};
		template <typename T>
		void operator()(const T &)
		{
			boost::mpl::for_each<size_types>(runner<T>());
		}
};

}

typedef piranha::debug_access<add_tag> add_tester;

BOOST_AUTO_TEST_CASE(array_key_add_test)
{
	boost::mpl::for_each<value_types>(add_tester());
}

struct trim_identify_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			key_type k0;
			symbol_set v1, v2;
			v1.add("x");
			k0.resize(1u);
			BOOST_CHECK_THROW(k0.trim_identify(v2,v2),std::invalid_argument);
			v2.add("y");
			k0.resize(2u);
			k0[0u] = T(1);
			k0[1u] = T(2);
			v2.add("x");
			k0.trim_identify(v1,v2);
			BOOST_CHECK(v1 == symbol_set());
			k0[0u] = T(0);
			v1.add("x");
			v1.add("y");
			k0.trim_identify(v1,v2);
			BOOST_CHECK(v1 == symbol_set({symbol("x")}));
			k0[0u] = T(0);
			k0[1u] = T(0);
			v1.add("y");
			k0.trim_identify(v1,v2);
			BOOST_CHECK(v1 == symbol_set({symbol("x"),symbol("y")}));
			k0[0u] = T(1);
			k0.trim_identify(v1,v2);
			BOOST_CHECK(v1 == symbol_set({symbol("y")}));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_trim_identify_test)
{
	boost::mpl::for_each<value_types>(trim_identify_tester());
}

struct trim_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			key_type k0;
			symbol_set v1, v2;
			v1.add("x");
			v1.add("y");
			v1.add("z");
			BOOST_CHECK_THROW(k0.trim(v2,v1),std::invalid_argument);
			k0 = key_type{T(1),T(2),T(3)};
			v2.add("x");
			BOOST_CHECK((k0.trim(v2,v1) == key_type{T(2),T(3)}));
			v2.add("z");
			v2.add("a");
			BOOST_CHECK((k0.trim(v2,v1) == key_type{T(2)}));
			v2.add("y");
			BOOST_CHECK((k0.trim(v2,v1) == key_type()));
			v2 = symbol_set();
			BOOST_CHECK((k0.trim(v2,v1) == k0));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_trim_test)
{
	boost::mpl::for_each<value_types>(trim_tester());
}

struct tt_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef g_key_type<T,U> key_type;
			BOOST_CHECK(is_hashable<key_type>::value);
			BOOST_CHECK(is_equality_comparable<key_type>::value);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(array_key_type_traits_test)
{

	boost::mpl::for_each<value_types>(tt_tester());
}

// Fake value type
struct fvt
{
	fvt() = default;
	fvt(int);
	fvt(const fvt &) = default;
	fvt(fvt &&) noexcept;
	fvt &operator=(const fvt &) = default;
	fvt &operator=(fvt &&) noexcept;
	bool operator<(const fvt &) const;
	bool operator==(const fvt &) const;
	bool operator!=(const fvt &) const;
};

// Fake value type with addition operation.
struct fvt2
{
	fvt2() = default;
	fvt2(int);
	fvt2(const fvt2 &) = default;
	fvt2(fvt2 &&) noexcept;
	fvt2 &operator=(const fvt2 &) = default;
	fvt2 &operator=(fvt2 &&) noexcept;
	bool operator<(const fvt2 &) const;
	bool operator==(const fvt2 &) const;
	bool operator!=(const fvt2 &) const;
	fvt2 operator+(const fvt2 &) const;
};

// Fake value type with wrong operation.
struct foo_t {};

struct fvt3
{
	fvt3() = default;
	fvt3(int);
	fvt3(const fvt3 &) = default;
	fvt3(fvt3 &&) noexcept;
	fvt3 &operator=(const fvt3 &) = default;
	fvt3 &operator=(fvt3 &&) noexcept;
	bool operator<(const fvt3 &) const;
	bool operator==(const fvt3 &) const;
	bool operator!=(const fvt3 &) const;
	foo_t operator+(const fvt3 &) const;
};

namespace std
{

template <>
struct hash<fvt>
{
	typedef size_t result_type;
	typedef fvt argument_type;
	result_type operator()(const argument_type &) const noexcept;
};

template <>
struct hash<fvt2>
{
	typedef size_t result_type;
	typedef fvt2 argument_type;
	result_type operator()(const argument_type &) const noexcept;
};

template <>
struct hash<fvt3>
{
	typedef size_t result_type;
	typedef fvt3 argument_type;
	result_type operator()(const argument_type &) const noexcept;
};

}

template <typename T>
using add_type = decltype(std::declval<T const &>().vector_add(std::declval<T &>(),std::declval<T const &>()));

template <typename T, typename = void>
struct has_add
{
	static const bool value = false;
};

template <typename T>
struct has_add<T,typename std::enable_if<std::is_same<add_type<T>,add_type<T>>::value>::type>
{
	static const bool value = true;
};

struct ae_tester
{
	template <typename T>
	void operator()(const T &)
	{
		typedef g_key_type<fvt,T> key_type;
		BOOST_CHECK(!has_add<key_type>::value);
		typedef g_key_type<fvt2,T> key_type2;
		BOOST_CHECK(has_add<key_type2>::value);
		typedef g_key_type<fvt3,T> key_type3;
		BOOST_CHECK(!has_add<key_type3>::value);
	}
};

BOOST_AUTO_TEST_CASE(array_key_add_enabler_test)
{

	boost::mpl::for_each<size_types>(ae_tester());
}
