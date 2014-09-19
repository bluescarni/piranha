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

#include "../src/base_term.hpp"

#define BOOST_TEST_MODULE base_term_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <numeric>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../src/environment.hpp"
#include "../src/forwarding.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

// NOTE: maybe once series coefficient is available we can add it here.
typedef boost::mpl::vector<double,integer,rational> cf_types;
typedef boost::mpl::vector<monomial<int>,monomial<integer>> key_types;

template <typename Cf, typename Key>
class g_term_type: public base_term<Cf,Key,g_term_type<Cf,Key>>
{
		typedef base_term<Cf,Key,g_term_type> base;
	public:
		g_term_type() = default;
		g_term_type(const g_term_type &) = default;
		g_term_type(g_term_type &&) = default;
		g_term_type &operator=(const g_term_type &) = default;
		g_term_type &operator=(g_term_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_term_type,base)
};

struct constructor_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			typedef g_term_type<Cf,Key> term_type;
			typedef typename Key::value_type value_type;
			symbol_set args;
			args.add("x");
			// Default constructor.
			BOOST_CHECK_EQUAL(term_type().m_cf,Cf());
			BOOST_CHECK(term_type().m_key == Key());
			// Generic constructor.
			BOOST_CHECK_EQUAL(term_type(Cf(1),Key{value_type(1)}).m_cf,Cf(1));
			BOOST_CHECK(term_type(Cf(1),Key{value_type(1)}).m_key == Key{value_type(1)});
			// Constructor from term of different type.
			typedef long Cf2;
			typedef g_term_type<Cf2,Key> other_term_type;
			other_term_type other(Cf2(1),Key{value_type(1)});
			BOOST_CHECK_EQUAL(term_type(Cf(other.m_cf),Key(other.m_key,args)).m_cf,Cf(Cf2(1)));
			BOOST_CHECK(term_type(Cf(other.m_cf),Key(other.m_key,args)).m_key[0] == Key{value_type(1)}[0]);
			// Move assignment.
			term_type term(Cf(1),Key{value_type(2)});
			term = term_type(Cf(2),Key{value_type(1)});
			BOOST_CHECK_EQUAL(term.m_cf,Cf(2));
			// Type trait test.
			BOOST_CHECK((std::is_constructible<term_type,Cf,Key>::value));
			BOOST_CHECK((!std::is_constructible<term_type,Cf,std::string>::value));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(base_term_constructor_test)
{
	environment env;
	boost::mpl::for_each<cf_types>(constructor_tester());
}

struct equality_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			typedef g_term_type<Cf,Key> term_type;
			typedef typename Key::value_type value_type;
			BOOST_CHECK(term_type() == term_type());
			BOOST_CHECK(term_type(Cf(1),Key{value_type(2)}) == term_type(Cf(2),Key{value_type(2)}));
			BOOST_CHECK(!(term_type(Cf(2),Key{value_type(1)}) == term_type(Cf(2),Key{value_type(2)})));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(base_term_equality_test)
{
	boost::mpl::for_each<cf_types>(equality_tester());
}

struct hash_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			typedef g_term_type<Cf,Key> term_type;
			typedef typename Key::value_type value_type;
			BOOST_CHECK_EQUAL(term_type().hash(),std::hash<Key>()(Key()));
			BOOST_CHECK_EQUAL(term_type(Cf(2),Key{value_type(1)}).hash(),std::hash<Key>()(Key{value_type(1)}));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(base_term_hash_test)
{
	boost::mpl::for_each<cf_types>(hash_tester());
}

struct compatibility_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			class term_type: public base_term<Cf,Key,term_type>
			{
				public:
					term_type() = default;
					term_type(const term_type &) = default;
					term_type(term_type &&) = default;
					term_type &operator=(const term_type &) = default;
					term_type &operator=(term_type &&) = default;
					explicit term_type(const Cf &, const Key &) {}
			};
			typedef typename Key::value_type value_type;
			symbol_set args;
			term_type t1;
			BOOST_CHECK_EQUAL(t1.is_compatible(args),t1.m_key.is_compatible(args));
			term_type t2;
			t2.m_cf = 1;
			t2.m_key = Key{value_type(1)};
			BOOST_CHECK_EQUAL(t2.is_compatible(args),t2.m_key.is_compatible(args));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(base_term_compatibility_test)
{
	boost::mpl::for_each<cf_types>(compatibility_tester());
}

struct ignorability_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			class term_type: public base_term<Cf,Key,term_type>
			{
				public:
					term_type() = default;
					term_type(const term_type &) = default;
					term_type(term_type &&) = default;
					term_type &operator=(const term_type &) = default;
					term_type &operator=(term_type &&) = default;
					explicit term_type(const Cf &, const Key &) {}
			};
			symbol_set args;
			term_type t1;
			BOOST_CHECK_EQUAL(t1.is_ignorable(args),(t1.m_key.is_ignorable(args) || math::is_zero(t1.m_cf)));
			BOOST_CHECK(t1.is_ignorable(args));
			term_type t2;
			t2.m_cf = 1;
			BOOST_CHECK_EQUAL(t2.is_ignorable(args),(t2.m_key.is_ignorable(args) || math::is_zero(t2.m_cf)));
			BOOST_CHECK(!t2.is_ignorable(args));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(base_term_ignorability_test)
{
	boost::mpl::for_each<cf_types>(ignorability_tester());
}

template <typename Cf, typename Key>
class term_type: public base_term<Cf,Key,term_type<Cf,Key>>
{
	public:
		term_type() = default;
		term_type(const term_type &) = default;
		term_type(term_type &&) = default;
		term_type &operator=(const term_type &) = default;
		term_type &operator=(term_type &&) = default;
		explicit term_type(const Cf &, const Key &) {}
};

template <typename Cf, typename Key>
class term_type_invalid1: public base_term<Cf,Key,term_type_invalid1<Cf,Key>>
{
	public:
		term_type_invalid1() = default;
		term_type_invalid1(const term_type_invalid1 &) = default;
		term_type_invalid1(term_type_invalid1 &&) = default;
		term_type_invalid1 &operator=(const term_type_invalid1 &) = default;
		term_type_invalid1 &operator=(term_type_invalid1 &&) = default;
};

template <typename Cf, typename Key>
class term_type_invalid2: public base_term<Cf,Key,term_type_invalid2<Cf,Key>>
{
	public:
		term_type_invalid2() = delete;
		term_type_invalid2(const term_type_invalid2 &) = default;
		term_type_invalid2(term_type_invalid2 &&) = default;
		term_type_invalid2 &operator=(const term_type_invalid2 &) = default;
		term_type_invalid2 &operator=(term_type_invalid2 &&) = default;
		explicit term_type_invalid2(const Cf &, const Key &) {}
};

template <typename Cf, typename Key>
class term_type_invalid3
{
	public:
		term_type_invalid3() = default;
		term_type_invalid3(const term_type_invalid3 &) = default;
		term_type_invalid3(term_type_invalid3 &&) = default;
		term_type_invalid3 &operator=(const term_type_invalid3 &) = default;
		term_type_invalid3 &operator=(term_type_invalid3 &&) = default;
		explicit term_type_invalid3(const Cf &, const Key &) {}
};

template <typename Cf, typename Key>
class term_type_invalid4: public base_term<Cf,Key,term_type_invalid4<Cf,Key>>
{
	public:
		term_type_invalid4() = default;
		term_type_invalid4(const term_type_invalid4 &) = default;
		term_type_invalid4(term_type_invalid4 &&) = default;
		term_type_invalid4 &operator=(const term_type_invalid4 &) = default;
		term_type_invalid4 &operator=(term_type_invalid4 &&) = default;
		explicit term_type_invalid4(Cf &, const Key &) {}
};

struct is_term_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			BOOST_CHECK((is_term<term_type<Cf,Key>>::value));
			BOOST_CHECK((!is_term<term_type_invalid1<Cf,Key>>::value));
			BOOST_CHECK((!is_term<term_type_invalid2<Cf,Key>>::value));
			BOOST_CHECK((!is_term<term_type_invalid3<Cf,Key>>::value));
			BOOST_CHECK((!is_term<term_type_invalid4<Cf,Key>>::value));
			BOOST_CHECK((!is_term<term_type<Cf,Key> &>::value));
			BOOST_CHECK((!is_term<const term_type<Cf,Key>>::value));
			BOOST_CHECK((!is_term<term_type<Cf,Key> const &>::value));
			BOOST_CHECK((!is_term<term_type<Cf,Key> &&>::value));
			BOOST_CHECK((!is_term<term_type<Cf,Key> *>::value));
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(base_term_is_term_test)
{
	boost::mpl::for_each<cf_types>(is_term_tester());
	BOOST_CHECK(!is_term<int>::value);
	BOOST_CHECK(!is_term<std::string>::value);
}
