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

#include "../src/substitutable_series.hpp"

#define BOOST_TEST_MODULE substitutable_series_test
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../src/config.hpp"
#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/forwarding.hpp"
#include "../src/is_key.hpp"
#include "../src/key_is_multipliable.hpp"
#include "../src/math.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"
#include "../src/serialization.hpp"
#include "../src/series.hpp"
#include "../src/symbol_set.hpp"
#include "../src/term.hpp"

using namespace piranha;

template <typename Cf, typename Key>
class g_series_type: public substitutable_series<series<Cf,Key,g_series_type<Cf,Key>>,g_series_type<Cf,Key>>
{
		using base = substitutable_series<series<Cf,Key,g_series_type<Cf,Key>>,g_series_type<Cf,Key>>;
		PIRANHA_SERIALIZE_THROUGH_BASE(base)
	public:
		template <typename Cf2>
		using rebind = g_series_type<Cf2,Key>;
		g_series_type() = default;
		g_series_type(const g_series_type &) = default;
		g_series_type(g_series_type &&) = default;
		explicit g_series_type(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_symbol_set.add(name);
			// Construct and insert the term.
			this->insert(term_type(Cf(1),typename term_type::key_type{typename Key::value_type(1)}));
		}
		g_series_type &operator=(const g_series_type &) = default;
		g_series_type &operator=(g_series_type &&) = default;
		PIRANHA_FORWARDING_CTOR(g_series_type,base)
		PIRANHA_FORWARDING_ASSIGNMENT(g_series_type,base)
};

// An alternative monomial class with no suitable subs() method.
template <class T>
class new_monomial: public monomial<T>
{
		using base = monomial<T>;
	public:
		using base::base;
		new_monomial merge_args(const symbol_set &orig_args, const symbol_set &new_args) const
		{
			auto tmp = static_cast<const base *>(this)->merge_args(orig_args,new_args);
			new_monomial retval;
			std::copy(tmp.begin(),tmp.end(),std::back_inserter(retval));
			return retval;
		}
		new_monomial trim(const symbol_set &trim_args, const symbol_set &orig_args) const
		{
			auto tmp = static_cast<const base *>(this)->trim(trim_args,orig_args);
			new_monomial retval;
			std::copy(tmp.begin(),tmp.end(),std::back_inserter(retval));
			return retval;
		}
		template <typename Cf>
		static void multiply(std::array<term<Cf,new_monomial>,base::multiply_arity> &res, const term<Cf,new_monomial> &t1,
			const term<Cf,new_monomial> &t2, const symbol_set &args)
		{
			term<Cf,new_monomial> &t = res[0u];
			if(unlikely(t1.m_key.size() != args.size())) {
				piranha_throw(std::invalid_argument,"invalid size of arguments set");
			}
			t.m_cf = t1.m_cf * t2.m_cf;
			t1.m_key.vector_add(t.m_key,t2.m_key);
		}
};

namespace std
{

template <typename T>
struct hash<new_monomial<T>>
{
	std::size_t operator()(const new_monomial<T> &m) const
	{
		return m.hash();
	}
};

}

BOOST_AUTO_TEST_CASE(subs_series_subs_test)
{
	environment env;
	// Substitution on key only.
	using stype0 = g_series_type<rational,monomial<int>>;
	// Type trait checks.
	BOOST_CHECK((has_subs<stype0,int>::value));
	BOOST_CHECK((has_subs<stype0,double>::value));
	BOOST_CHECK((has_subs<stype0,integer>::value));
	BOOST_CHECK((has_subs<stype0,rational>::value));
	BOOST_CHECK((has_subs<stype0,real>::value));
	BOOST_CHECK((!has_subs<stype0,std::string>::value));
	{
	stype0 x{"x"}, y{"y"}, z{"z"};
	auto tmp = (x + y).subs("x",2);
	BOOST_CHECK_EQUAL(tmp,y + 2);
	BOOST_CHECK(tmp.is_identical(math::subs(x+y,"x",2)));
	BOOST_CHECK(tmp.is_identical(y + 2 + x - x));
	BOOST_CHECK((std::is_same<decltype(tmp),stype0>::value));
	auto tmp2 = (x + y).subs("x",2.);
	BOOST_CHECK_EQUAL(tmp2,y + 2.);
	BOOST_CHECK(tmp2.is_identical(math::subs(x+y,"x",2.)));
	BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<double,monomial<int>>>::value));
	auto tmp3 = (3*x + y*y/7).subs("y",2/5_q);
	BOOST_CHECK(tmp3.is_identical(math::subs(3*x + y*y/7,"y",2/5_q)));
	BOOST_CHECK((std::is_same<decltype(tmp3),stype0>::value));
	BOOST_CHECK_EQUAL(tmp3,3*x + 2/5_q * 2/5_q / 7);
	auto tmp4 = (3*x + y*y/7).subs("y",2.123_r);
	BOOST_CHECK(tmp4.is_identical(math::subs(3*x + y*y/7,"y",2.123_r)));
	BOOST_CHECK((std::is_same<decltype(tmp4),g_series_type<real,monomial<int>>>::value));
	BOOST_CHECK_EQUAL(tmp4,3*x + math::pow(2.123_r,2) / 7);
	auto tmp5 = (3*x + y*y/7).subs("y",-2_z);
	BOOST_CHECK(tmp5.is_identical(math::subs(3*x + y*y/7,"y",-2_z)));
	BOOST_CHECK((std::is_same<decltype(tmp5),stype0>::value));
	BOOST_CHECK_EQUAL(tmp5,3*x + math::pow(-2_z,2) / 7_q);
	// Substitution with series.
	auto tmp6 = (3*x + y*y/7).subs("y",z*2);
	BOOST_CHECK(tmp6.is_identical(math::subs(3*x + y*y/7,"y",z*2)));
	BOOST_CHECK((std::is_same<decltype(tmp6),stype0>::value));
	BOOST_CHECK_EQUAL(tmp6,3*x + 4*z*z/7);
	}
	// Subs on cf only.
	using stype1 = g_series_type<stype0,new_monomial<int>>;
	BOOST_CHECK((is_key<stype1::term_type::key_type>::value));
	BOOST_CHECK((key_is_multipliable<rational,stype1::term_type::key_type>::value));
	BOOST_CHECK((!key_has_subs<stype1::term_type::key_type,rational>::value));
	BOOST_CHECK((has_subs<stype1,int>::value));
	BOOST_CHECK((has_subs<stype1,double>::value));
	BOOST_CHECK((has_subs<stype1,integer>::value));
	BOOST_CHECK((has_subs<stype1,rational>::value));
	BOOST_CHECK((has_subs<stype1,real>::value));
	BOOST_CHECK((!has_subs<stype1,std::string>::value));
	{
	stype1 x{stype0{"x"}}, y{stype0{"y"}}, z{stype0{"z"}};
	auto tmp = (x + y).subs("x",2);
	BOOST_CHECK_EQUAL(tmp,y + 2);
	BOOST_CHECK(tmp.is_identical(math::subs(x+y,"x",2)));
	BOOST_CHECK(tmp.is_identical(y + 2 + x - x));
	BOOST_CHECK((std::is_same<decltype(tmp),stype1>::value));
	auto tmp2 = (x + y).subs("x",2.);
	BOOST_CHECK_EQUAL(tmp2,y + 2.);
	BOOST_CHECK(tmp2.is_identical(math::subs(x+y,"x",2.)));
	BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,monomial<int>>,new_monomial<int>>>::value));
	auto tmp3 = (3*x + y*y/7).subs("y",2/5_q);
	BOOST_CHECK(tmp3.is_identical(math::subs(3*x + y*y/7,"y",2/5_q)));
	BOOST_CHECK((std::is_same<decltype(tmp3),stype1>::value));
	BOOST_CHECK_EQUAL(tmp3,3*x + 2/5_q * 2/5_q / 7);
	auto tmp4 = (3*x + y*y/7).subs("y",2.123_r);
	BOOST_CHECK(tmp4.is_identical(math::subs(3*x + y*y/7,"y",2.123_r)));
	BOOST_CHECK((std::is_same<decltype(tmp4),g_series_type<g_series_type<real,monomial<int>>,new_monomial<int>>>::value));
	BOOST_CHECK_EQUAL(tmp4,3*x + math::pow(2.123_r,2) / 7);
	auto tmp5 = (3*x + y*y/7).subs("y",-2_z);
	BOOST_CHECK(tmp5.is_identical(math::subs(3*x + y*y/7,"y",-2_z)));
	BOOST_CHECK((std::is_same<decltype(tmp5),stype1>::value));
	BOOST_CHECK_EQUAL(tmp5,3*x + math::pow(-2_z,2) / 7_q);
	auto tmp6 = (3*x + y*y/7).subs("y",-2*z);
	BOOST_CHECK(tmp6.is_identical(math::subs(3*x + y*y/7,"y",-2*z)));
	BOOST_CHECK((std::is_same<decltype(tmp6),stype1>::value));
	BOOST_CHECK_EQUAL(tmp6,3*x + 4*z*z/7);
	}
	// Subs on cf and key.
	using stype2 = g_series_type<stype0,monomial<int>>;
	BOOST_CHECK((is_key<stype2::term_type::key_type>::value));
	BOOST_CHECK((key_is_multipliable<rational,stype2::term_type::key_type>::value));
	BOOST_CHECK((key_has_subs<stype2::term_type::key_type,rational>::value));
	BOOST_CHECK((has_subs<stype2,int>::value));
	BOOST_CHECK((has_subs<stype2,double>::value));
	BOOST_CHECK((has_subs<stype2,integer>::value));
	BOOST_CHECK((has_subs<stype2,rational>::value));
	BOOST_CHECK((has_subs<stype2,real>::value));
	BOOST_CHECK((!has_subs<stype2,std::string>::value));
	{
	// Recursive poly with x and y at the first level, z in the second.
	stype2 x{stype0{"x"}}, y{stype0{"y"}}, z{"z"}, t{"t"};
	auto tmp = ((x + y)*z).subs("x",2);
	BOOST_CHECK_EQUAL(tmp,(2 + y)*z);
	BOOST_CHECK(tmp.is_identical(math::subs((x + y)*z,"x",2)));
	BOOST_CHECK((std::is_same<decltype(tmp),stype2>::value));
	auto tmp2 = ((x + y)*z).subs("x",2.);
	BOOST_CHECK_EQUAL(tmp2,(2. + y)*z);
	BOOST_CHECK(tmp2.is_identical(math::subs((x + y)*z,"x",2.)));
	BOOST_CHECK((std::is_same<decltype(tmp2),g_series_type<g_series_type<double,monomial<int>>,monomial<int>>>::value));
	auto tmp3 = ((3*x + y*y/7)*z).subs("z",2/5_q);
	BOOST_CHECK(tmp3.is_identical(math::subs((3*x + y*y/7)*z,"z",2/5_q)));
	BOOST_CHECK((std::is_same<decltype(tmp3),stype2>::value));
	BOOST_CHECK_EQUAL(tmp3,(3*x + y*y/7) * 2/5_q);
	auto tmp4 = ((3*x + y*y/7)*z).subs("y",2/3_q).subs("z",4_z);
	BOOST_CHECK(tmp4.is_identical(math::subs(math::subs((3*x + y*y/7)*z,"y",2/3_q),"z",4_z)));
	BOOST_CHECK((std::is_same<decltype(tmp4),stype2>::value));
	BOOST_CHECK_EQUAL(tmp4,(3*x+2/3_q*2/3_q/7)*4_z);
	auto tmp5 = ((3*x + y*y/7)*z).subs("y",-2.123_r);
	BOOST_CHECK(tmp5.is_identical(math::subs((3*x + y*y/7)*z,"y",-2.123_r)));
	BOOST_CHECK((std::is_same<decltype(tmp5),g_series_type<g_series_type<real,monomial<int>>,monomial<int>>>::value));
	BOOST_CHECK_EQUAL(tmp5,(3*x + math::pow(-2.123_r,2)/7)*z);
	auto tmp6 = ((3*x + y*y/7)*z).subs("z",2*t);
	BOOST_CHECK(tmp6.is_identical(math::subs((3*x + y*y/7)*z,"z",2*t)));
	BOOST_CHECK((std::is_same<decltype(tmp6),stype2>::value));
	BOOST_CHECK_EQUAL(tmp6,(3*x + y*y/7)*2*t);
	}
}

BOOST_AUTO_TEST_CASE(subs_series_serialization_test)
{
	using stype = g_series_type<rational,monomial<int>>;
	stype x("x"), y("y"), z = (x + 3*y + 1).pow(4), tmp;
	std::stringstream ss;
	{
	boost::archive::text_oarchive oa(ss);
	oa << z;
	}
	{
	boost::archive::text_iarchive ia(ss);
	ia >> tmp;
	}
	BOOST_CHECK_EQUAL(z,tmp);
}
