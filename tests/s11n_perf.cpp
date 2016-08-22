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

#include "../src/s11n.hpp"

#define BOOST_TEST_MODULE s11n_test
#include <boost/test/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>

#include "../src/config.hpp"
#include "../src/exceptions.hpp"
#include "../src/init.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/polynomial.hpp"
#include "pearce1.hpp"

using namespace piranha;
namespace bfs = boost::filesystem;

struct tmp_file {
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

BOOST_AUTO_TEST_CASE(s11n_series_memory_test)
{
    init();
    std::cout << "Multiplication time: ";
    const auto res = pearce1<integer, monomial<signed char>>();
    std::cout << '\n';
    using pt = decltype(res * res);
    std::stringstream ss;
    {
        boost::archive::binary_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        boost_save(oa, res);
        std::cout << "Boost save, binary, timing: ";
    }
    std::cout << "Boost save, binary, size: " << ss.tellp() << '\n';
    pt tmp;
    {
        boost::archive::binary_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        boost_load(ia, tmp);
        std::cout << "Boost load, binary, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
    std::cout << '\n';
    {
        boost::archive::text_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        boost_save(oa, res);
        std::cout << "Boost save, text, timing: ";
    }
    std::cout << "Boost save, text, size: " << ss.tellp() << '\n';
    tmp = pt{};
    {
        boost::archive::text_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        boost_load(ia, tmp);
        std::cout << "Boost load, text, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
    std::cout << '\n';
    {
        boost::archive::text_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        oa << res;
        std::cout << "Old save, text, timing: ";
    }
    std::cout << "Old save, text, size: " << ss.tellp() << '\n';
    tmp = pt{};
    {
        boost::archive::text_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        ia >> tmp;
        std::cout << "Old load, text, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
    std::cout << '\n';
    {
        boost::archive::binary_oarchive oa(ss);
        boost::timer::auto_cpu_timer t;
        oa << res;
        std::cout << "Old save, binary, timing: ";
    }
    std::cout << "Old save, binary, size: " << ss.tellp() << '\n';
    tmp = pt{};
    {
        boost::archive::binary_iarchive ia(ss);
        boost::timer::auto_cpu_timer t;
        ia >> tmp;
        std::cout << "Old load, binary, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    ss.str("");
    ss.clear();
    std::cout << '\n';
#if defined(PIRANHA_WITH_MSGPACK)
    msgpack::sbuffer sbuf;
    {
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        boost::timer::auto_cpu_timer t;
        msgpack_pack(p, res, msgpack_format::binary);
        std::cout << "msgpack pack, sbuffer, binary, timing: ";
    }
    std::cout << "msgpack pack, binary, size: " << sbuf.size() << '\n';
    {
        boost::timer::auto_cpu_timer t;
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        msgpack_convert(tmp, oh.get(), msgpack_format::binary);
        std::cout << "msgpack convert, sbuffer, binary, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    sbuf.clear();
    std::cout << '\n';
    {
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        boost::timer::auto_cpu_timer t;
        msgpack_pack(p, res, msgpack_format::portable);
        std::cout << "msgpack pack, sbuffer, portable, timing: ";
    }
    std::cout << "msgpack pack, portable, size: " << sbuf.size() << '\n';
    {
        boost::timer::auto_cpu_timer t;
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        msgpack_convert(tmp, oh.get(), msgpack_format::portable);
        std::cout << "msgpack convert, sbuffer, portable, timing: ";
    }
    BOOST_CHECK_EQUAL(tmp, res);
    sbuf.clear();
    std::cout << '\n';
#endif
}

static inline std::ifstream::pos_type filesize(const std::string &filename)
{
    std::ifstream in(filename.c_str(), std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

BOOST_AUTO_TEST_CASE(s11n_series_file_test)
{
    std::cout << "Multiplication time: ";
    const auto res = pearce1<integer, monomial<signed char>>();
    std::cout << '\n';
    using pt = decltype(res * res);
    pt tmp;
    for (auto f : {data_format::boost_binary, data_format::boost_portable, data_format::msgpack_binary,
                   data_format::msgpack_portable}) {
        for (auto c : {compression::none, compression::bzip2, compression::gzip, compression::zlib}) {
            auto fn = static_cast<int>(f);
            auto cn = static_cast<int>(c);
            tmp_file file;
            try {
                boost::timer::auto_cpu_timer t;
                save_file(res, file.name(), f, c);
                std::cout << "File save, " << fn << ", " << cn << ": ";
            } catch (const not_implemented_error &) {
                std::cout << "Not supported: " << fn << ", " << cn << '\n';
                continue;
            }
            {
                boost::timer::auto_cpu_timer t;
                load_file(tmp, file.name(), f, c);
                std::cout << "File load, " << fn << ", " << cn << ": ";
            }
            std::cout << "File size, " << fn << ", " << cn << ": " << filesize(file.name()) << '\n';
            BOOST_CHECK_EQUAL(tmp, res);
            std::cout << '\n';
        }
    }
}
