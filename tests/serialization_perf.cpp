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

#define BOOST_TEST_MODULE serialization_test
#include <boost/test/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>
#include <sstream>
#include <string>

#include "../src/environment.hpp"
#include "../src/kronecker_monomial.hpp"
#include "../src/serialization.hpp"
#include "pearce1.hpp"

namespace bfs = boost::filesystem;

// Small raii class for creating a tmp file.
struct tmp_file
{
	tmp_file()
	{
		m_path = bfs::temp_directory_path();
		// Concatenate with a unique filename.
		m_path /= bfs::unique_path();
	}
	~tmp_file()
	{
		bfs::remove(m_path);
	}
	std::string name() const
	{
		return m_path.string();
	}
	bfs::path m_path;
};

using namespace piranha;

BOOST_AUTO_TEST_CASE(serialization_test_00)
{
	environment env;
	std::stringstream ss;
	std::cout << "Timing double multiplication:\n";
	auto ret1 = pearce1<double,k_monomial>();
	auto ret2(ret1);
	{
	boost::archive::text_oarchive oa(ss);
	boost::timer::auto_cpu_timer t;
	oa << ret1;
	std::cout << "Raw text serialization: ";
	}
	{
	boost::archive::text_iarchive ia(ss);
	boost::timer::auto_cpu_timer t;
	ia >> ret1;
	std::cout << "Raw text deserialization: ";
	}
	BOOST_CHECK(ret1 == ret2);
	std::cout << "\n\n";
}

BOOST_AUTO_TEST_CASE(serialization_test_01)
{
	std::cout << "Timing double multiplication:\n";
	using pt = polynomial<double,k_monomial>;
	auto ret1 = pearce1<double,k_monomial>();
	auto ret2(ret1);
	{
	tmp_file f;
	std::cout << "Filename: " << f.name() << '\n';
	{
	boost::timer::auto_cpu_timer t;
	pt::save(ret1,f.name(),file_format::binary);
	std::cout << "Raw binary file save: ";
	}
	{
	boost::timer::auto_cpu_timer t;
	ret1 = pt::load(f.name(),file_format::binary);
	std::cout << "Raw binary file load: ";
	}
	std::cout << "File size: " << bfs::file_size(f.m_path) / 1024. / 1024. << '\n';
	}
	BOOST_CHECK(ret1 == ret2);
	std::cout << "\n\n";
}

BOOST_AUTO_TEST_CASE(serialization_test_02)
{
	std::cout << "Timing double multiplication:\n";
	using pt = polynomial<double,k_monomial>;
	auto ret1 = pearce1<double,k_monomial>();
	auto ret2(ret1);
	{
	tmp_file f;
	std::cout << "Filename: " << f.name() << '\n';
	{
	boost::timer::auto_cpu_timer t;
	pt::save(ret1,f.name(),file_format::binary,file_compression::bzip2);
	std::cout << "Compressed binary file save: ";
	}
	{
	boost::timer::auto_cpu_timer t;
	ret1 = pt::load(f.name(),file_format::binary,file_compression::bzip2);
	std::cout << "Compressed binary file load: ";
	}
	std::cout << "File size: " << bfs::file_size(f.m_path) / 1024. / 1024. << '\n';
	}
	BOOST_CHECK(ret1 == ret2);
	std::cout << "\n\n";
}

BOOST_AUTO_TEST_CASE(serialization_test_03)
{
	std::cout << "Timing double multiplication:\n";
	using pt = polynomial<double,k_monomial>;
	auto ret1 = pearce1<double,k_monomial>();
	auto ret2(ret1);
	{
	tmp_file f;
	std::cout << "Filename: " << f.name() << '\n';
	{
	boost::timer::auto_cpu_timer t;
	pt::save(ret1,f.name());
	std::cout << "Raw text file save: ";
	}
	{
	boost::timer::auto_cpu_timer t;
	ret1 = pt::load(f.name());
	std::cout << "Raw text file load: ";
	}
	std::cout << "File size: " << bfs::file_size(f.m_path) / 1024. / 1024. << '\n';
	}
	BOOST_CHECK(ret1 == ret2);
	std::cout << "\n\n";
}

BOOST_AUTO_TEST_CASE(serialization_test_04)
{
	std::cout << "Timing double multiplication:\n";
	using pt = polynomial<double,k_monomial>;
	auto ret1 = pearce1<double,k_monomial>();
	auto ret2(ret1);
	{
	tmp_file f;
	std::cout << "Filename: " << f.name() << '\n';
	{
	boost::timer::auto_cpu_timer t;
	pt::save(ret1,f.name(),file_compression::bzip2);
	std::cout << "Compressed text file save: ";
	}
	{
	boost::timer::auto_cpu_timer t;
	ret1 = pt::load(f.name(),file_compression::bzip2);
	std::cout << "Compressed text file load: ";
	}
	std::cout << "File size: " << bfs::file_size(f.m_path) / 1024. / 1024. << '\n';
	}
	BOOST_CHECK(ret1 == ret2);
	std::cout << "\n\n";
}
