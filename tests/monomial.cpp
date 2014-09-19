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

#include "../src/monomial.hpp"

#define BOOST_TEST_MODULE monomial_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <cstddef>
#include <initializer_list>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "../src/environment.hpp"
#include "../src/exceptions.hpp"
#include "../src/math.hpp"
#include "../src/mp_integer.hpp"
#include "../src/mp_rational.hpp"
#include "../src/real.hpp"
#include "../src/symbol_set.hpp"
#include "../src/symbol.hpp"
#include "../src/type_traits.hpp"

using namespace piranha;

typedef boost::mpl::vector<signed char,short,int,integer> expo_types;
typedef boost::mpl::vector<std::integral_constant<std::size_t,0u>,std::integral_constant<std::size_t,1u>,std::integral_constant<std::size_t,5u>,
	std::integral_constant<std::size_t,10u>> size_types;

// Constructors, assignments and element access.
struct constructor_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> monomial_type;
			BOOST_CHECK(is_key<monomial_type>::value);
			monomial_type m0;
			BOOST_CHECK_NO_THROW(monomial_type tmp = monomial_type());
			BOOST_CHECK_NO_THROW(monomial_type tmp = monomial_type(monomial_type()));
			BOOST_CHECK_NO_THROW(monomial_type tmp(m0));
			// From init list.
			monomial_type m1{T(0),T(1),T(2),T(3)};
			BOOST_CHECK_EQUAL(m1.size(),static_cast<decltype(m1.size())>(4));
			for (typename monomial_type::size_type i = 0; i < m1.size(); ++i) {
				BOOST_CHECK_EQUAL(m1[i],T(i));
				BOOST_CHECK_NO_THROW(m1[i] = static_cast<T>(T(i) + T(1)));
				BOOST_CHECK_EQUAL(m1[i],T(i) + T(1));
			}
			monomial_type m1a{0,1,2,3};
			BOOST_CHECK_EQUAL(m1a.size(),static_cast<decltype(m1a.size())>(4));
			for (typename monomial_type::size_type i = 0; i < m1a.size(); ++i) {
				BOOST_CHECK_EQUAL(m1a[i],T(i));
				BOOST_CHECK_NO_THROW(m1a[i] = static_cast<T>(T(i) + T(1)));
				BOOST_CHECK_EQUAL(m1a[i],T(i) + T(1));
			}
			BOOST_CHECK_NO_THROW(m0 = m1);
			BOOST_CHECK_NO_THROW(m0 = std::move(m1));
			// Constructor from arguments vector.
			monomial_type m2 = monomial_type(symbol_set{});
			BOOST_CHECK_EQUAL(m2.size(),unsigned(0));
			monomial_type m3 = monomial_type(symbol_set({symbol("a"),symbol("b"),symbol("c")}));
			BOOST_CHECK_EQUAL(m3.size(),unsigned(3));
			symbol_set vs({symbol("a"),symbol("b"),symbol("c")});
			monomial_type k2(vs);
			BOOST_CHECK_EQUAL(k2.size(),vs.size());
			BOOST_CHECK_EQUAL(k2[0],T(0));
			BOOST_CHECK_EQUAL(k2[1],T(0));
			BOOST_CHECK_EQUAL(k2[2],T(0));
			// Generic constructor for use in series.
			BOOST_CHECK_THROW(monomial_type tmp(k2,symbol_set{}),std::invalid_argument);
			monomial_type k3(k2,vs);
			BOOST_CHECK_EQUAL(k3.size(),vs.size());
			BOOST_CHECK_EQUAL(k3[0],T(0));
			BOOST_CHECK_EQUAL(k3[1],T(0));
			BOOST_CHECK_EQUAL(k3[2],T(0));
			monomial_type k4(monomial_type(vs),vs);
			BOOST_CHECK_EQUAL(k4.size(),vs.size());
			BOOST_CHECK_EQUAL(k4[0],T(0));
			BOOST_CHECK_EQUAL(k4[1],T(0));
			BOOST_CHECK_EQUAL(k4[2],T(0));
			typedef monomial<int,U> monomial_type2;
			monomial_type2 k5(vs);
			BOOST_CHECK_THROW(monomial_type tmp(k5,symbol_set{}),std::invalid_argument);
			monomial_type k6(k5,vs);
			BOOST_CHECK_EQUAL(k6.size(),vs.size());
			BOOST_CHECK_EQUAL(k6[0],T(0));
			BOOST_CHECK_EQUAL(k6[1],T(0));
			BOOST_CHECK_EQUAL(k6[2],T(0));
			monomial_type k7(monomial_type2(vs),vs);
			BOOST_CHECK_EQUAL(k7.size(),vs.size());
			BOOST_CHECK_EQUAL(k7[0],T(0));
			BOOST_CHECK_EQUAL(k7[1],T(0));
			BOOST_CHECK_EQUAL(k7[2],T(0));
			// Type trait check.
			BOOST_CHECK((std::is_constructible<monomial_type,monomial_type>::value));
			BOOST_CHECK((std::is_constructible<monomial_type,symbol_set>::value));
			BOOST_CHECK((!std::is_constructible<monomial_type,std::string>::value));
			BOOST_CHECK((!std::is_constructible<monomial_type,monomial_type,int>::value));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_constructor_test)
{
	environment env;
	boost::mpl::for_each<expo_types>(constructor_tester());
}

struct hash_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> monomial_type;
			monomial_type m0;
			BOOST_CHECK_EQUAL(m0.hash(),std::size_t());
			BOOST_CHECK_EQUAL(m0.hash(),std::hash<monomial_type>()(m0));
			monomial_type m1{T(0),T(1),T(2),T(3)};
			BOOST_CHECK_EQUAL(m1.hash(),std::hash<monomial_type>()(m1));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_hash_test)
{
	boost::mpl::for_each<expo_types>(hash_tester());
}

struct compatibility_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> monomial_type;
			monomial_type m0;
			BOOST_CHECK(m0.is_compatible(symbol_set{}));
			symbol_set ss({symbol("foobarize")});
			monomial_type m1{T(0),T(1)};
			BOOST_CHECK(!m1.is_compatible(ss));
			monomial_type m2{T(0)};
			BOOST_CHECK(m2.is_compatible(ss));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_compatibility_test)
{
	boost::mpl::for_each<expo_types>(compatibility_tester());
}

struct ignorability_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> monomial_type;
			monomial_type m0;
			BOOST_CHECK(!m0.is_ignorable(symbol_set{}));
			monomial_type m1{T(0)};
			BOOST_CHECK(!m1.is_ignorable(symbol_set({symbol("foobarize")})));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_ignorability_test)
{
	boost::mpl::for_each<expo_types>(ignorability_tester());
}

struct merge_args_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> key_type;
			symbol_set v1, v2;
			v2.add("a");
			key_type k;
			auto out = k.merge_args(v1,v2);
			BOOST_CHECK_EQUAL(out.size(),unsigned(1));
			BOOST_CHECK_EQUAL(out[0],T(0));
			v2.add(symbol("b"));
			v2.add(symbol("c"));
			v2.add(symbol("d"));
			v1.add(symbol("b"));
			v1.add(symbol("d"));
			k.push_back(T(2));
			k.push_back(T(4));
			out = k.merge_args(v1,v2);
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
			out = k.merge_args(v1,v2);
			BOOST_CHECK_EQUAL(out.size(),unsigned(8));
			BOOST_CHECK_EQUAL(out[0],T(0));
			BOOST_CHECK_EQUAL(out[1],T(2));
			BOOST_CHECK_EQUAL(out[2],T(0));
			BOOST_CHECK_EQUAL(out[3],T(4));
			BOOST_CHECK_EQUAL(out[4],T(5));
			BOOST_CHECK_EQUAL(out[5],T(0));
			BOOST_CHECK_EQUAL(out[6],T(7));
			BOOST_CHECK_EQUAL(out[7],T(0));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_merge_args_test)
{
	boost::mpl::for_each<expo_types>(merge_args_tester());
}

struct is_unitary_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> key_type;
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
			BOOST_CHECK_THROW(k2.is_unitary(symbol_set{}),std::invalid_argument);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_is_unitary_test)
{
	boost::mpl::for_each<expo_types>(is_unitary_tester());
}

struct degree_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> key_type;
			key_type k0;
			symbol_set v;
			BOOST_CHECK_EQUAL(k0.degree(v),T(0));
			BOOST_CHECK_EQUAL(k0.ldegree(v),T(0));
			v.add(symbol("a"));
			key_type k1(v);
			BOOST_CHECK_EQUAL(k1.degree(v),T(0));
			BOOST_CHECK_EQUAL(k1.ldegree(v),T(0));
			k1[0] = T(2);
			BOOST_CHECK_EQUAL(k1.degree(v),T(2));
			BOOST_CHECK_EQUAL(k1.ldegree(v),T(2));
			v.add(symbol("b"));
			key_type k2(v);
			BOOST_CHECK_EQUAL(k2.degree(v),T(0));
			BOOST_CHECK_EQUAL(k2.ldegree(v),T(0));
			k2[0] = T(2);
			k2[1] = T(3);
			BOOST_CHECK_EQUAL(k2.degree(v),T(2) + T(3));
			BOOST_CHECK_THROW(k2.degree(symbol_set{}),std::invalid_argument);
			// Partial degree.
			BOOST_CHECK(k2.degree(std::set<std::string>{},v) == T(0));
			BOOST_CHECK(k2.degree({"a"},v) == T(2));
			BOOST_CHECK(k2.degree({"A"},v) == T(0));
			BOOST_CHECK(k2.degree({"z"},v) == T(0));
			BOOST_CHECK(k2.degree({"z","A","a"},v) == T(2));
			BOOST_CHECK(k2.degree({"z","A","b"},v) == T(3));
			BOOST_CHECK(k2.degree({"B","A","b"},v) == T(3));
			BOOST_CHECK(k2.degree({"a","b","z"},v) == T(3) + T(2));
			BOOST_CHECK(k2.degree({"a","b","A"},v) == T(3) + T(2));
			BOOST_CHECK(k2.degree({"a","b","A","z"},v) == T(3) + T(2));
			BOOST_CHECK(k2.ldegree(std::set<std::string>{},v) == T(0));
			BOOST_CHECK(k2.ldegree({"a"},v) == T(2));
			BOOST_CHECK(k2.ldegree({"A"},v) == T(0));
			BOOST_CHECK(k2.ldegree({"z"},v) == T(0));
			BOOST_CHECK(k2.ldegree({"z","A","a"},v) == T(2));
			BOOST_CHECK(k2.ldegree({"z","A","b"},v) == T(3));
			BOOST_CHECK(k2.ldegree({"B","A","b"},v) == T(3));
			BOOST_CHECK(k2.ldegree({"a","b","z"},v) == T(3) + T(2));
			BOOST_CHECK(k2.ldegree({"a","b","A"},v) == T(3) + T(2));
			BOOST_CHECK(k2.ldegree({"a","b","A","z"},v) == T(3) + T(2));
			v.add(symbol("c"));
			key_type k3(v);
			k3[0] = T(2);
			k3[1] = T(3);
			k3[2] = T(4);
			BOOST_CHECK(k3.degree({"a","b","A","z"},v) == T(3) + T(2));
			BOOST_CHECK(k3.degree({"a","c","A","z"},v) == T(4) + T(2));
			BOOST_CHECK(k3.degree({"a","c","b","z"},v) == T(4) + T(2) + T(3));
			BOOST_CHECK(k3.degree({"a","c","b","A"},v) == T(4) + T(2) + T(3));
			BOOST_CHECK(k3.degree({"c","b","A"},v) == T(4) + T(3));
			BOOST_CHECK(k3.degree({"A","B","C"},v) == T(0));
			BOOST_CHECK(k3.degree({"x","y","z"},v) == T(0));
			BOOST_CHECK(k3.degree({"x","y","z","A","B","C","a"},v) == T(2));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_degree_test)
{
	boost::mpl::for_each<expo_types>(degree_tester());
}

struct multiply_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1({0}), k2({1}), retval;
			BOOST_CHECK_THROW(k1.multiply(retval,k2,vs),std::invalid_argument);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_multiply_test)
{
	boost::mpl::for_each<expo_types>(multiply_tester());
}

struct print_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1;
			std::ostringstream oss;
			k1.print(oss,vs);
			BOOST_CHECK(oss.str().empty());
			vs.add("x");
			k_type k2(vs);
			k2.print(oss,vs);
			BOOST_CHECK(oss.str() == "");
			oss.str("");
			k_type k3({T(-1)});
			k3.print(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"x**-1");
			k_type k4({T(1)});
			oss.str("");
			k4.print(oss,vs);
			BOOST_CHECK(oss.str() == "x");
			k_type k5({T(-1),T(1)});
			vs.add("y");
			oss.str("");
			k5.print(oss,vs);
			BOOST_CHECK(oss.str() == "x**-1*y");
			k_type k6({T(-1),T(-2)});
			oss.str("");
			k6.print(oss,vs);
			BOOST_CHECK(oss.str() == "x**-1*y**-2");
			k_type k7;
			BOOST_CHECK_THROW(k7.print(oss,vs),std::invalid_argument);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_print_test)
{
	boost::mpl::for_each<expo_types>(print_tester());
}

struct linear_argument_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			BOOST_CHECK_THROW(k_type().linear_argument(vs),std::invalid_argument);
			vs.add("x");
			BOOST_CHECK_THROW(k_type().linear_argument(vs),std::invalid_argument);
			k_type k({T(1)});
			BOOST_CHECK_EQUAL(k.linear_argument(vs),"x");
			k = k_type({T(0),T(1)});
			vs.add("y");
			BOOST_CHECK_EQUAL(k.linear_argument(vs),"y");
			k = k_type({T(0),T(2)});
			BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
			k = k_type({T(2),T(0)});
			BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
			k = k_type({T(1),T(1)});
			BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_linear_argument_test)
{
	boost::mpl::for_each<expo_types>(linear_argument_tester());
	typedef monomial<rational> k_type;
	symbol_set vs;
	vs.add("x");
	k_type k({rational(1,2)});
	BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
	k = k_type({rational(1),rational(0)});
	vs.add("y");
	BOOST_CHECK_EQUAL(k.linear_argument(vs),"x");
	k = k_type({rational(2),rational(1)});
	BOOST_CHECK_THROW(k.linear_argument(vs),std::invalid_argument);
}

struct pow_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1;
			BOOST_CHECK(k1 == k1.pow(42,vs));
			vs.add("x");
			BOOST_CHECK_THROW(k1.pow(42,vs),std::invalid_argument);
			k1 = k_type({T(1),T(2),T(3)});
			vs.add("y");
			vs.add("z");
			BOOST_CHECK(k1.pow(2,vs) == k_type({T(2),T(4),T(6)}));
			BOOST_CHECK(k1.pow(-2,vs) == k_type({T(-2),T(-4),T(-6)}));
			BOOST_CHECK(k1.pow(0,vs) == k_type({T(0),T(0),T(0)}));
			vs.add("a");
			BOOST_CHECK_THROW(k1.pow(42,vs),std::invalid_argument);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_pow_test)
{
	boost::mpl::for_each<expo_types>(pow_tester());
}

struct partial_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1;
			vs.add("x");
			BOOST_CHECK_THROW(k1.partial(symbol("x"),vs),std::invalid_argument);
			k1 = k_type({T(2)});
			auto ret = k1.partial(symbol("x"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(2));
			BOOST_CHECK(ret.second == k_type({T(1)}));
			ret = k1.partial(symbol("y"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(0));
			k1 = k_type({T(0)});
			ret = k1.partial(symbol("x"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(0));
			vs.add("y");
			k1 = k_type({T(-1),T(0)});
			ret = k1.partial(symbol("y"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(0));
			ret = k1.partial(symbol("x"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(-1));
			BOOST_CHECK(ret.second == k_type({T(-2),T(0)}));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_partial_test)
{
	boost::mpl::for_each<expo_types>(partial_tester());
}

struct evaluate_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			BOOST_CHECK((key_is_evaluable<k_type,integer>::value));
			typedef std::unordered_map<symbol,integer> dict_type;
			symbol_set vs;
			k_type k1;
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type{},vs),integer(1));
			vs.add("x");
			BOOST_CHECK_THROW(k1.evaluate(dict_type{},vs),std::invalid_argument);
			k1 = k_type({T(1)});
			BOOST_CHECK_THROW(k1.evaluate(dict_type{},vs),std::invalid_argument);
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(1)}},vs),1);
			k1 = k_type({T(2)});
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(3)}},vs),9);
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(3)},{symbol("y"),integer(4)}},vs),9);
			k1 = k_type({T(2),T(4)});
			vs.add("y");
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("x"),integer(3)},{symbol("y"),integer(4)}},vs),2304);
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type{{symbol("y"),integer(4)},{symbol("x"),integer(3)}},vs),2304);
			typedef std::unordered_map<symbol,double> dict_type2;
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type2{{symbol("y"),-4.3},{symbol("x"),3.2}},vs),math::pow(3.2,2) * math::pow(-4.3,4));
			typedef std::unordered_map<symbol,rational> dict_type3;
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type3{{symbol("y"),rational(1,2)},{symbol("x"),rational(-4,3)}},vs),
				math::pow(rational(4,-3),2) * math::pow(rational(-1,-2),4));
			k1 = k_type({T(-2),T(-4)});
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type3{{symbol("y"),rational(1,2)},{symbol("x"),rational(-4,3)}},vs),
				math::pow(rational(4,-3),-2) * math::pow(rational(-1,-2),-4));
			typedef std::unordered_map<symbol,real> dict_type4;
			BOOST_CHECK_EQUAL(k1.evaluate(dict_type4{{symbol("y"),real(1.234)},{symbol("x"),real(5.678)}},vs),
				math::pow(real(5.678),-2) * math::pow(real(1.234),-4));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_evaluate_test)
{
	boost::mpl::for_each<expo_types>(evaluate_tester());
	BOOST_CHECK((key_is_evaluable<monomial<rational>,double>::value));
	BOOST_CHECK((key_is_evaluable<monomial<rational>,real>::value));
	BOOST_CHECK((!key_is_evaluable<monomial<rational>,std::string>::value));
	BOOST_CHECK((!key_is_evaluable<monomial<rational>,void *>::value));
}

struct subs_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1;
			auto ret = k1.subs(symbol("x"),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK((std::is_same<integer,decltype(ret.first)>::value));
			BOOST_CHECK(ret.second == k1);
			vs.add("x");
			BOOST_CHECK_THROW(k1.subs(symbol("x"),integer(4),vs),std::invalid_argument);
			k1 = k_type({T(2)});
			ret = k1.subs(symbol("y"),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK(ret.second == k1);
			ret = k1.subs(symbol("x"),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(4),T(2)));
			BOOST_CHECK(ret.second == k_type());
			k1 = k_type({T(2),T(3)});
			BOOST_CHECK_THROW(k1.subs(symbol("x"),integer(4),vs),std::invalid_argument);
			vs.add("y");
			ret = k1.subs(symbol("y"),integer(-2),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(-2),T(3)));
			BOOST_CHECK(ret.second == k_type{T(2)});
			auto ret2 = k1.subs(symbol("x"),real(-2.345),vs);
			BOOST_CHECK_EQUAL(ret2.first,math::pow(real(-2.345),T(2)));
			BOOST_CHECK(ret2.second == k_type{T(3)});
			BOOST_CHECK((std::is_same<real,decltype(ret2.first)>::value));
			auto ret3 = k1.subs(symbol("x"),rational(-1,2),vs);
			BOOST_CHECK_EQUAL(ret3.first,rational(1,4));
			BOOST_CHECK(ret3.second == k_type{T(3)});
			BOOST_CHECK((std::is_same<rational,decltype(ret3.first)>::value));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_subs_test)
{
	boost::mpl::for_each<expo_types>(subs_tester());
}

struct print_tex_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1;
			std::ostringstream oss;
			k1.print_tex(oss,vs);
			BOOST_CHECK(oss.str().empty());
			k1 = k_type({T(0)});
			BOOST_CHECK_THROW(k1.print_tex(oss,vs),std::invalid_argument);
			vs.add("x");
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"");
			k1 = k_type({T(1)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"{x}");
			oss.str("");
			k1 = k_type({T(-1)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{x}}");
			oss.str("");
			k1 = k_type({T(2)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"{x}^{2}");
			oss.str("");
			k1 = k_type({T(-2)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{x}^{2}}");
			vs.add("y");
			oss.str("");
			k1 = k_type({T(-2),T(1)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"\\frac{{y}}{{x}^{2}}");
			oss.str("");
			k1 = k_type({T(-2),T(3)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"\\frac{{y}^{3}}{{x}^{2}}");
			oss.str("");
			k1 = k_type({T(-2),T(-3)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{x}^{2}{y}^{3}}");
			oss.str("");
			k1 = k_type({T(2),T(3)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"{x}^{2}{y}^{3}");
			oss.str("");
			k1 = k_type({T(1),T(3)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"{x}{y}^{3}");
			oss.str("");
			k1 = k_type({T(0),T(3)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"{y}^{3}");
			oss.str("");
			k1 = k_type({T(0),T(0)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"");
			oss.str("");
			k1 = k_type({T(0),T(1)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"{y}");
			oss.str("");
			k1 = k_type({T(0),T(-1)});
			k1.print_tex(oss,vs);
			BOOST_CHECK_EQUAL(oss.str(),"\\frac{1}{{y}}");
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_print_tex_test)
{
	boost::mpl::for_each<expo_types>(print_tex_tester());
}

struct integrate_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1;
			auto ret = k1.integrate(symbol("a"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(1));
			BOOST_CHECK(ret.second == k_type({T(1)}));
			vs.add("b");
			BOOST_CHECK_THROW(k1.integrate(symbol("b"),vs),std::invalid_argument);
			k1 = k_type{T(1)};
			ret = k1.integrate(symbol("b"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(2));
			BOOST_CHECK(ret.second == k_type({T(2)}));
			k1 = k_type{T(2)};
			ret = k1.integrate(symbol("c"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(1));
			BOOST_CHECK(ret.second == k_type({T(2),T(1)}));
			ret = k1.integrate(symbol("a"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(1));
			BOOST_CHECK(ret.second == k_type({T(1),T(2)}));
			k1 = k_type{T(2),T(3)};
			vs.add("d");
			ret = k1.integrate(symbol("a"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(1));
			BOOST_CHECK(ret.second == k_type({T(1),T(2),T(3)}));
			ret = k1.integrate(symbol("b"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(3));
			BOOST_CHECK(ret.second == k_type({T(3),T(3)}));
			ret = k1.integrate(symbol("c"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(1));
			BOOST_CHECK(ret.second == k_type({T(2),T(1),T(3)}));
			ret = k1.integrate(symbol("d"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(4));
			BOOST_CHECK(ret.second == k_type({T(2),T(4)}));
			ret = k1.integrate(symbol("e"),vs);
			BOOST_CHECK_EQUAL(ret.first,T(1));
			BOOST_CHECK(ret.second == k_type({T(2),T(3),T(1)}));
			k1 = k_type{T(-1),T(3)};
			BOOST_CHECK_THROW(k1.integrate(symbol("b"),vs),std::invalid_argument);
			k1 = k_type{T(2),T(-1)};
			BOOST_CHECK_THROW(k1.integrate(symbol("d"),vs),std::invalid_argument);
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_integrate_test)
{
	boost::mpl::for_each<expo_types>(integrate_tester());
}

struct ipow_subs_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			symbol_set vs;
			k_type k1;
			auto ret = k1.ipow_subs(symbol("x"),integer(45),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK((std::is_same<integer,decltype(ret.first)>::value));
			BOOST_CHECK(ret.second == k1);
			vs.add("x");
			BOOST_CHECK_THROW(k1.ipow_subs(symbol("x"),integer(35),integer(4),vs),std::invalid_argument);
			k1 = k_type({T(2)});
			ret = k1.ipow_subs(symbol("y"),integer(2),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK(ret.second == k1);
			ret = k1.ipow_subs(symbol("x"),integer(1),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(4),T(2)));
			BOOST_CHECK(ret.second == k_type({T(0)}));
			ret = k1.ipow_subs(symbol("x"),integer(2),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(4),T(1)));
			BOOST_CHECK(ret.second == k_type({T(0)}));
			ret = k1.ipow_subs(symbol("x"),integer(-1),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK(ret.second == k_type({T(2)}));
			ret = k1.ipow_subs(symbol("x"),integer(4),integer(4),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK(ret.second == k_type({T(2)}));
			k1 = k_type({T(7),T(2)});
			BOOST_CHECK_THROW(k1.ipow_subs(symbol("x"),integer(4),integer(4),vs),std::invalid_argument);
			vs.add("y");
			ret = k1.ipow_subs(symbol("x"),integer(3),integer(2),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(2)));
			BOOST_CHECK((ret.second == k_type{T(1),T(2)}));
			ret = k1.ipow_subs(symbol("x"),integer(4),integer(2),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(1)));
			BOOST_CHECK((ret.second == k_type{T(3),T(2)}));
			ret = k1.ipow_subs(symbol("x"),integer(-4),integer(2),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK((ret.second == k_type{T(7),T(2)}));
			k1 = k_type({T(-7),T(2)});
			ret = k1.ipow_subs(symbol("x"),integer(4),integer(2),vs);
			BOOST_CHECK_EQUAL(ret.first,1);
			BOOST_CHECK((ret.second == k_type{T(-7),T(2)}));
			ret = k1.ipow_subs(symbol("x"),integer(-4),integer(2),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(1)));
			BOOST_CHECK((ret.second == k_type{T(-3),T(2)}));
			ret = k1.ipow_subs(symbol("x"),integer(-3),integer(2),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(2)));
			BOOST_CHECK((ret.second == k_type{T(-1),T(2)}));
			k1 = k_type({T(2),T(-7)});
			ret = k1.ipow_subs(symbol("y"),integer(-3),integer(2),vs);
			BOOST_CHECK_EQUAL(ret.first,math::pow(integer(2),T(2)));
			BOOST_CHECK((ret.second == k_type{T(2),T(-1)}));
			BOOST_CHECK_THROW(k1.ipow_subs(symbol("y"),integer(0),integer(2),vs),zero_division_error);
			k1 = k_type({T(-7),T(2)});
			auto ret2 = k1.ipow_subs(symbol("x"),integer(-4),real(-2.345),vs);
			BOOST_CHECK_EQUAL(ret2.first,math::pow(real(-2.345),T(1)));
			BOOST_CHECK((ret2.second == k_type{T(-3),T(2)}));
			BOOST_CHECK((std::is_same<real,decltype(ret2.first)>::value));
			auto ret3 = k1.ipow_subs(symbol("x"),integer(-3),rational(-1,2),vs);
			BOOST_CHECK_EQUAL(ret3.first,math::pow(rational(-1,2),T(2)));
			BOOST_CHECK((ret3.second == k_type{T(-1),T(2)}));
			BOOST_CHECK((std::is_same<rational,decltype(ret3.first)>::value));
		}
	};
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_ipow_subs_test)
{
	boost::mpl::for_each<expo_types>(ipow_subs_tester());
}

struct tt_tester
{
	template <typename T>
	struct runner
	{
		template <typename U>
		void operator()(const U &)
		{
			typedef monomial<T,U> k_type;
			BOOST_CHECK((!key_has_t_subs<k_type,int,int>::value));
			BOOST_CHECK((!key_has_t_subs<k_type &,int,int>::value));
			BOOST_CHECK((!key_has_t_subs<k_type &&,int,int>::value));
			BOOST_CHECK((!key_has_t_subs<const k_type &,int,int>::value));
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
	template <typename T>
	void operator()(const T &)
	{
		boost::mpl::for_each<size_types>(runner<T>());
	}
};

BOOST_AUTO_TEST_CASE(monomial_type_traits_test)
{
	boost::mpl::for_each<expo_types>(tt_tester());
}
