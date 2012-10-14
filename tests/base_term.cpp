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
#include <tuple>
#include <type_traits>
#include <utility>

#include "../src/config.hpp"
#include "../src/detail/inherit.hpp"
#include "../src/environment.hpp"
#include "../src/integer.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/rational.hpp"
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
		g_term_type &operator=(g_term_type &&other) piranha_noexcept_spec(true)
		{
			base_term<Cf,Key,g_term_type>::operator=(std::move(other));
			return *this;
		}
		// Needed to satisfy concept checking.
		PIRANHA_USING_CTOR(g_term_type,base)
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
					term_type &operator=(term_type &&other) piranha_noexcept_spec(true)
					{
						base_term<Cf,Key,term_type>::operator=(std::move(other));
						return *this;
					}
					// Needed to satisfy concept checking.
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
					term_type &operator=(term_type &&other) piranha_noexcept_spec(true)
					{
						base_term<Cf,Key,term_type>::operator=(std::move(other));
						return *this;
					}
					// Needed to satisfy concept checking.
					explicit term_type(const Cf &, const Key &) {}
			};
			typedef typename Key::value_type value_type;
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
