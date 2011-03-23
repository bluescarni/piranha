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
#include <tuple>
#include <utility>

#include "../src/echelon_descriptor.hpp"
#include "../src/integer.hpp"
#include "../src/monomial.hpp"
#include "../src/numerical_coefficient.hpp"

using namespace piranha;

// NOTE: maybe once series coefficient is available we can add it here.
typedef boost::mpl::vector<numerical_coefficient<double>,numerical_coefficient<integer>> cf_types;
typedef boost::mpl::vector<monomial<int>,monomial<integer>> key_types;

struct constructor_tester
{
	template <typename Cf>
	struct runner
	{
		template <typename Key>
		void operator()(const Key &)
		{
			typedef base_term<Cf,Key,void> term_type;
			typedef typename Key::value_type value_type;
			// Default constructor.
			BOOST_CHECK_EQUAL(term_type().m_cf.get_value(),Cf().get_value());
			BOOST_CHECK_EQUAL(term_type().m_key,Key());
			// Generic constructor.
			BOOST_CHECK_EQUAL(term_type(1,Key{value_type(1)}).m_cf.get_value(),Cf(1).get_value());
			BOOST_CHECK_EQUAL(term_type(1,Key{value_type(1)}).m_key,Key{value_type(1)});
			// Constructor from term of different type.
			typedef numerical_coefficient<long> Cf2;
			typedef base_term<Cf2,Key,void> other_term_type;
			other_term_type other(1,Key{value_type(1)});
			BOOST_CHECK_EQUAL(term_type(other).m_cf.get_value(),Cf2(1).get_value());
			BOOST_CHECK_EQUAL(term_type(other).m_key[0],Key{value_type(1)}[0]);
			// Move assignment.
			term_type term(1,Key{value_type(2)});
			term = term_type(2,Key{value_type(1)});
			BOOST_CHECK_EQUAL(term.m_cf.get_value(),Cf(2).get_value());
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
			typedef base_term<Cf,Key,void> term_type;
			typedef typename Key::value_type value_type;
			BOOST_CHECK(term_type() == term_type());
			BOOST_CHECK(term_type(1,Key{value_type(2)}) == term_type(2,Key{value_type(2)}));
			BOOST_CHECK(!(term_type(2,Key{value_type(1)}) == term_type(2,Key{value_type(2)})));
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
			typedef base_term<Cf,Key,void> term_type;
			typedef typename Key::value_type value_type;
			BOOST_CHECK_EQUAL(term_type().hash(),std::hash<Key>()(Key()));
			BOOST_CHECK_EQUAL(term_type(2,Key{value_type(1)}).hash(),std::hash<Key>()(Key{value_type(1)}));
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
			class term_type: public base_term<Cf,Key,term_type> {};
			typedef typename Key::value_type value_type;
			echelon_descriptor<term_type> ed;
			const auto &args = ed.template get_args<term_type>();
			term_type t1;
			BOOST_CHECK_EQUAL((int)(t1.is_compatible(ed)) - (int)(t1.m_cf.is_compatible(ed) && t1.m_key.is_compatible(args)),0);
			term_type t2;
			t2.m_cf = 1;
			t2.m_key = Key{value_type(1)};
			BOOST_CHECK_EQUAL((int)(t2.is_compatible(ed)) - (int)(t2.m_cf.is_compatible(ed) && t2.m_key.is_compatible(args)),0);
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
			class term_type: public base_term<Cf,Key,term_type> {};
			typedef typename Key::value_type value_type;
			echelon_descriptor<term_type> ed;
			const auto &args = ed.template get_args<term_type>();
			term_type t1;
			BOOST_CHECK_EQUAL((int)(t1.is_ignorable(ed)) - (int)(t1.m_cf.is_ignorable(ed) || t1.m_key.is_ignorable(args)),0);
			term_type t2;
			t2.m_cf = 1;
			BOOST_CHECK_EQUAL((int)(t2.is_ignorable(ed)) - (int)(t2.m_cf.is_ignorable(ed) || t2.m_key.is_ignorable(args)),0);
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
