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

#include "../src/term.hpp"

#define BOOST_TEST_MODULE term_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <functional>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>

#include "../src/environment.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/symbol_set.hpp"

static const int ntries = 1000;
std::mt19937 rng;

using namespace piranha;

// NOTE: maybe once series coefficient is available we can add it here.
typedef boost::mpl::vector<double,integer,rational> cf_types;
typedef boost::mpl::vector<monomial<int>,monomial<integer>> key_types;

struct constructor_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			typedef term<Cf,Key> term_type;
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
			typedef term<Cf2,Key> other_term_type;
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

BOOST_AUTO_TEST_CASE(term_constructor_test)
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
			typedef term<Cf,Key> term_type;
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

BOOST_AUTO_TEST_CASE(term_equality_test)
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
			typedef term<Cf,Key> term_type;
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

BOOST_AUTO_TEST_CASE(term_hash_test)
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
			using term_type = term<Cf,Key>;
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

BOOST_AUTO_TEST_CASE(term_compatibility_test)
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
			using term_type = term<Cf,Key>;
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

BOOST_AUTO_TEST_CASE(term_ignorability_test)
{
	boost::mpl::for_each<cf_types>(ignorability_tester());
}

struct serialization_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			typedef term<Cf,Key> term_type;
			term_type tmp;
			std::uniform_int_distribution<int> int_dist(std::numeric_limits<int>::min(),
				std::numeric_limits<int>::max());
			std::uniform_int_distribution<unsigned> size_dist(0u,10u);
			for (int i = 0; i < ntries; ++i) {
				Cf cf(int_dist(rng));
				Key key;
				const auto size = size_dist(rng);
				for (unsigned j = 0u; j < size; ++j) {
					key.push_back(typename Key::value_type(int_dist(rng)));
				}
				term_type t(std::move(cf),std::move(key));
				std::stringstream ss;
				{
				boost::archive::text_oarchive oa(ss);
				oa << t;
				}
				{
				boost::archive::text_iarchive ia(ss);
				ia >> tmp;
				}
				BOOST_CHECK(tmp.m_cf == t.m_cf);
				BOOST_CHECK(tmp.m_key == t.m_key);
			}
		}
	};
	template <typename Cf>
	void operator()(const Cf &)
	{
		boost::mpl::for_each<key_types>(runner<Cf>());
	}
};

BOOST_AUTO_TEST_CASE(term_serialization_test)
{
	boost::mpl::for_each<cf_types>(serialization_tester());
}
