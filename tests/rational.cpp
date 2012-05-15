/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani, Hartmuth Gutsche                          *
 *   bluescarni@gmail.com 
 *   hgutsche@xplornet.com
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
#include "../src/integer.hpp"
#include "../src/rational.hpp"


#define BOOST_TEST_MODULE rational_test
#include <boost/test/unit_test.hpp>
//#include <boost/>

//const boost::fusion::vector<char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double,long double> numerator_values(
//	(char)-42,(short)42,-42,42L,-42LL,
//	(unsigned char)42,(unsigned short)42,42U,42UL,42ULL,
//	23.456f,-23.456,23.456L
//);
//
//
//const boost::fusion::vector<char,short,int,long,long long,unsigned char,unsigned short,unsigned,unsigned long,unsigned long long,float,double,long double> denumerator_values(
//	(char)-42,(short)42,-42,42L,-42LL,
//	(unsigned char)42,(unsigned short)42,42U,42UL,42ULL,
//	23.456f,-23.456,23.456L
//);


BOOST_AUTO_TEST_CASE(rational_constructor_tests)
{
	// temporary just to make some preliminary tests
	piranha::rational rat1();
	piranha::integer  denum(1);
	piranha::integer  numer(2);
	piranha::rational rat2(numer, denum);
	piranha::rational rat3(rat2);
    denum = 8;
    numer = 6;
    piranha::rational rat4(numer, denum);
    piranha::rational("123/456");
//    piranha::rational("1/-1");
    piranha::rational rat5(3, 4);
    piranha::rational rat6(6);

}
