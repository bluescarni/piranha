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

#include <algorithm>
#include <array>
#include <atomic>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/version.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/config.hpp"
#include "../src/detail/demangle.hpp"
#include "../src/exceptions.hpp"
#include "../src/init.hpp"
#include "../src/is_key.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

namespace bfs = boost::filesystem;

// Small raii class for creating a tmp file.
// NOTE: this will not actually create the file, it will just create
// a tmp file name - so one is supposed to use the m_path member to create a file
// in the usual way. The destructor will attempt to delete the file at m_path, nothing
// will happen if the file does not exist.
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

static const int ntrials = 1000;

using integral_types = boost::mpl::vector<char, signed char, short, int, long, long long, unsigned char, unsigned short,
                                          unsigned, unsigned long, unsigned long long>;

using fp_types = boost::mpl::vector<float, double, long double>;

// Helper function to roundtrip the the (de)serialization of type T via boost serialization.
template <typename T>
static inline T boost_roundtrip(const T &x)
{
    std::ostringstream oss;
    {
        boost::archive::text_oarchive oa(oss);
        boost_save(oa, x);
    }
    T retval;
    std::istringstream iss;
    iss.str(oss.str());
    {
        boost::archive::text_iarchive ia(iss);
        boost_load(ia, retval);
    }
    return retval;
}

struct unserial {
};

// A good saving archive.
struct sa0 {
    using self_t = sa0;
    using is_loading = boost::mpl::bool_<false>;
    using is_saving = boost::mpl::bool_<true>;
    // Disable serialization for struct unserial.
    template <typename T, typename std::enable_if<!std::is_same<T, unserial>::value, int>::type = 0>
    self_t &operator<<(const T &);
    template <typename T>
    self_t &operator&(const T &);
    template <typename T>
    void save_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
};

// Missing methods.
struct sa1 {
    using self_t = sa1;
    using is_loading = boost::mpl::bool_<false>;
    using is_saving = boost::mpl::bool_<true>;
    template <typename T>
    self_t &operator<<(const T &);
    template <typename T>
    self_t &operator&(const T &);
    template <typename T>
    void save_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    // template <typename Helper>
    // void get_helper(void * const = nullptr) const;
};

struct sa2 {
    using self_t = sa2;
    // using is_loading = boost::mpl::bool_<false>;
    using is_saving = boost::mpl::bool_<true>;
    template <typename T>
    self_t &operator<<(const T &);
    template <typename T>
    self_t &operator&(const T &);
    template <typename T>
    void save_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
};

struct sa3 {
    using self_t = sa3;
    using is_loading = boost::mpl::bool_<true>;
    using is_saving = boost::mpl::bool_<true>;
    template <typename T>
    self_t &operator<<(const T &);
    template <typename T>
    self_t &operator&(const T &);
    template <typename T>
    void save_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
};

struct sa4 {
    using self_t = sa4;
    using is_loading = boost::mpl::bool_<false>;
    using is_saving = boost::mpl::bool_<true>;
    template <typename T>
    self_t &operator<<(const T &);
    template <typename T>
    self_t &operator&(const T &);
    template <typename T>
    void save_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version();
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
};

// A good loading archive.
struct la0 {
    using self_t = la0;
    using is_loading = boost::mpl::bool_<true>;
    using is_saving = boost::mpl::bool_<false>;
    // Disable serialization for struct unserial.
    template <typename T, typename std::enable_if<!std::is_same<T, unserial>::value, int>::type = 0>
    self_t &operator>>(T &);
    template <typename T>
    self_t &operator&(T &);
    template <typename T>
    void load_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
    template <typename T>
    void reset_object_address(T *, T *);
    void delete_created_pointers();
};

struct la1 {
    using self_t = la1;
    using is_loading = boost::mpl::bool_<true>;
    using is_saving = boost::mpl::bool_<false>;
    // template <typename T, typename std::enable_if<!std::is_same<T,unserial>::value,int>::type = 0>
    // self_t &operator>>(T &);
    template <typename T>
    self_t &operator&(T &);
    template <typename T>
    void load_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
    template <typename T>
    void reset_object_address(T *, T *);
    void delete_created_pointers();
};

struct la2 {
    using self_t = la2;
    using is_loading = boost::mpl::bool_<true>;
    using is_saving = boost::mpl::bool_<false>;
    template <typename T, typename std::enable_if<!std::is_same<T, unserial>::value, int>::type = 0>
    self_t &operator>>(T &);
    template <typename T>
    void operator&(T &);
    template <typename T>
    void load_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
    template <typename T>
    void reset_object_address(T *, T *);
    void delete_created_pointers();
};

struct la3 {
    using self_t = la3;
    using is_loading = boost::mpl::bool_<true>;
    using is_saving = boost::mpl::bool_<false>;
    template <typename T, typename std::enable_if<!std::is_same<T, unserial>::value, int>::type = 0>
    self_t &operator>>(T &);
    template <typename T>
    self_t &operator&(T &);
    template <typename T>
    void load_binary(T *, std::size_t);
    // template <typename T>
    // void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
    template <typename T>
    void reset_object_address(T *, T *);
    void delete_created_pointers();
};

struct la4 {
    using self_t = la4;
    using is_loading = boost::mpl::bool_<true>;
    using is_saving = boost::mpl::bool_<false>;
    template <typename T, typename std::enable_if<!std::is_same<T, unserial>::value, int>::type = 0>
    self_t &operator>>(T &);
    template <typename T>
    self_t &operator&(T &);
    template <typename T>
    void load_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
    template <typename T>
    void reset_object_address(T *, T *);
    // void delete_created_pointers();
};

struct la5 {
    using self_t = la5;
    using is_loading = boost::mpl::bool_<true>;
    using is_saving = boost::mpl::bool_<false>;
    template <typename T, typename std::enable_if<!std::is_same<T, unserial>::value, int>::type = 0>
    self_t &operator>>(T &);
    template <typename T>
    self_t &operator&(T &);
    template <typename T>
    void load_binary(T *, std::size_t);
    template <typename T>
    void register_type();
    unsigned get_library_version() const;
    template <typename Helper>
    void get_helper(void *const = nullptr) const;
    // template <typename T>
    // void reset_object_address(T *, T *);
    void delete_created_pointers();
};

// A key with Boost ser support.
struct keya {
    keya() = default;
    keya(const keya &) = default;
    keya(keya &&) noexcept;
    keya &operator=(const keya &) = default;
    keya &operator=(keya &&) noexcept;
    keya(const symbol_set &);
    bool operator==(const keya &) const;
    bool operator!=(const keya &) const;
    bool is_compatible(const symbol_set &) const noexcept;
    bool is_ignorable(const symbol_set &) const noexcept;
    keya merge_args(const symbol_set &, const symbol_set &) const;
    bool is_unitary(const symbol_set &) const;
    void print(std::ostream &, const symbol_set &) const;
    void print_tex(std::ostream &, const symbol_set &) const;
    template <typename T, typename U>
    std::vector<std::pair<std::string, keya>> t_subs(const std::string &, const T &, const U &,
                                                     const symbol_set &) const;
    void trim_identify(symbol_set &, const symbol_set &) const;
    keya trim(const symbol_set &, const symbol_set &) const;
    template <typename Archive, typename std::enable_if<is_boost_saving_archive<Archive, int>::value, int>::type = 0>
    int boost_save(Archive &, const symbol_set &) const;
    template <typename Archive, typename std::enable_if<is_boost_loading_archive<Archive, int>::value, int>::type = 0>
    void boost_load(Archive &, const symbol_set &);
};

// A key without Boost ser support.
struct keyb {
    keyb() = default;
    keyb(const keyb &) = default;
    keyb(keyb &&) noexcept;
    keyb &operator=(const keyb &) = default;
    keyb &operator=(keyb &&) noexcept;
    keyb(const symbol_set &);
    bool operator==(const keyb &) const;
    bool operator!=(const keyb &) const;
    bool is_compatible(const symbol_set &) const noexcept;
    bool is_ignorable(const symbol_set &) const noexcept;
    keyb merge_args(const symbol_set &, const symbol_set &) const;
    bool is_unitary(const symbol_set &) const;
    void print(std::ostream &, const symbol_set &) const;
    void print_tex(std::ostream &, const symbol_set &) const;
    template <typename T, typename U>
    std::vector<std::pair<std::string, keyb>> t_subs(const std::string &, const T &, const U &,
                                                     const symbol_set &) const;
    void trim_identify(symbol_set &, const symbol_set &) const;
    keyb trim(const symbol_set &, const symbol_set &) const;
    template <typename Archive, typename std::enable_if<is_boost_saving_archive<Archive, int>::value, int>::type = 0>
    int boost_save(Archive &, const symbol_set &);
    template <typename Archive, typename std::enable_if<is_boost_loading_archive<Archive, int>::value, int>::type = 0>
    void boost_load(Archive &);
    template <typename Archive, typename std::enable_if<is_boost_saving_archive<Archive, int>::value, int>::type = 0>
    int boost_save(Archive &, symbol_set &) const;
};

namespace std
{

template <>
struct hash<keya> {
    std::size_t operator()(const keya &) const;
};

template <>
struct hash<keyb> {
    std::size_t operator()(const keyb &) const;
};
}

BOOST_AUTO_TEST_CASE(s11n_test_boost_tt)
{
    init();
    // Saving archive.
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive, int>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive, std::string>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive, int *>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive, int const *>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive, int &&>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive, const int &>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive &, int>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive &, int &>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive &, const int &>::value));
    BOOST_CHECK((!is_boost_saving_archive<const boost::archive::binary_oarchive &, int>::value));
    BOOST_CHECK((!is_boost_saving_archive<const boost::archive::binary_oarchive, int>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::binary_oarchive &&, int>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::text_oarchive, int>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::text_oarchive &, int>::value));
    BOOST_CHECK((!is_boost_saving_archive<const boost::archive::text_oarchive &, int>::value));
    BOOST_CHECK((is_boost_saving_archive<boost::archive::text_oarchive &&, int>::value));
    BOOST_CHECK((!is_boost_saving_archive<boost::archive::binary_oarchive, void>::value));
    BOOST_CHECK((!is_boost_saving_archive<void, void>::value));
    BOOST_CHECK((!is_boost_saving_archive<void, int>::value));
    // Loading archive.
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive, int>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive, std::string>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive, int *>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive, int &&>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive &&, int>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive &, int &>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive &, int>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive &, int &>::value));
    BOOST_CHECK((!is_boost_loading_archive<const boost::archive::binary_iarchive &, int &>::value));
    BOOST_CHECK((!is_boost_loading_archive<boost::archive::binary_iarchive &, const int &>::value));
    BOOST_CHECK((!is_boost_loading_archive<boost::archive::binary_iarchive, const int &>::value));
    BOOST_CHECK((!is_boost_loading_archive<const boost::archive::binary_iarchive &, int>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::binary_iarchive &&, int>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::text_iarchive, int>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::text_iarchive &, int>::value));
    BOOST_CHECK((!is_boost_loading_archive<const boost::archive::text_iarchive &, int>::value));
    BOOST_CHECK((is_boost_loading_archive<boost::archive::text_iarchive &&, int>::value));
    BOOST_CHECK((!is_boost_loading_archive<boost::archive::binary_iarchive, void>::value));
    BOOST_CHECK((!is_boost_loading_archive<void, void>::value));
    BOOST_CHECK((!is_boost_loading_archive<void, int>::value));
    // Test custom archives.
    BOOST_CHECK((is_boost_saving_archive<sa0, int>::value));
    BOOST_CHECK((!is_boost_saving_archive<sa0, unserial>::value));
#if BOOST_VERSION >= 105700
    BOOST_CHECK((!is_boost_saving_archive<sa1, int>::value));
#endif
    BOOST_CHECK((!is_boost_saving_archive<sa2, int>::value));
    BOOST_CHECK((!is_boost_saving_archive<sa3, int>::value));
    BOOST_CHECK((!is_boost_saving_archive<sa4, int>::value));
    BOOST_CHECK((is_boost_loading_archive<la0, int>::value));
    BOOST_CHECK((!is_boost_loading_archive<la0, unserial>::value));
    BOOST_CHECK((!is_boost_loading_archive<la1, int>::value));
    BOOST_CHECK((!is_boost_loading_archive<la2, int>::value));
    BOOST_CHECK((!is_boost_loading_archive<la3, int>::value));
    BOOST_CHECK((!is_boost_loading_archive<la4, int>::value));
    BOOST_CHECK((!is_boost_loading_archive<la5, int>::value));
    // Serialization funcs type traits.
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, int>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, long double>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, int &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, const int &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &, const int &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &&, const int &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &&, std::string>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &&, std::string &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive &&, std::string const &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive const &, const int &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive const &, std::string>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive const, const int &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_oarchive, wchar_t>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, int>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::binary_iarchive, void>::value));
    BOOST_CHECK((!has_boost_save<void, void>::value));
    BOOST_CHECK((!has_boost_save<void, int>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, int>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, long double>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, int &>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, std::string>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, std::string &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, const std::string>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, const std::string &>::value));
    BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive, int &>::value));
    BOOST_CHECK((!has_boost_load<const boost::archive::binary_iarchive &, int &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, const int &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &, const int &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive &&, const int &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive const &, const int &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive const, const int &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_iarchive, wchar_t>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, int>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::binary_oarchive, void>::value));
    BOOST_CHECK((!has_boost_load<void, void>::value));
    BOOST_CHECK((!has_boost_load<void, int>::value));
    // Key type traits.
    BOOST_CHECK(is_key<keya>::value);
    BOOST_CHECK(is_key<keyb>::value);
    BOOST_CHECK((key_has_boost_save<boost::archive::binary_oarchive, keya>::value));
    BOOST_CHECK((key_has_boost_save<boost::archive::binary_oarchive &, keya>::value));
    BOOST_CHECK((key_has_boost_save<boost::archive::binary_oarchive &, keya &>::value));
    BOOST_CHECK((key_has_boost_save<boost::archive::binary_oarchive &, const keya &>::value));
    BOOST_CHECK((key_has_boost_save<boost::archive::binary_oarchive &, const keya>::value));
    BOOST_CHECK((!key_has_boost_save<boost::archive::binary_iarchive &, keya>::value));
    BOOST_CHECK((!key_has_boost_save<const boost::archive::binary_oarchive, keya>::value));
    BOOST_CHECK((!key_has_boost_save<const boost::archive::binary_oarchive &, keya>::value));
    BOOST_CHECK((!key_has_boost_save<const boost::archive::binary_oarchive &, keya>::value));
    BOOST_CHECK((key_has_boost_load<boost::archive::binary_iarchive, keya>::value));
    BOOST_CHECK((key_has_boost_load<boost::archive::binary_iarchive &, keya>::value));
    BOOST_CHECK((key_has_boost_load<boost::archive::binary_iarchive &, keya &>::value));
    BOOST_CHECK((!key_has_boost_load<boost::archive::binary_iarchive &, const keya &>::value));
    BOOST_CHECK((!key_has_boost_load<boost::archive::binary_iarchive &, const keya>::value));
    BOOST_CHECK((!key_has_boost_load<boost::archive::binary_oarchive &, keya>::value));
    BOOST_CHECK((!key_has_boost_load<const boost::archive::binary_iarchive, keya>::value));
    BOOST_CHECK((!key_has_boost_load<const boost::archive::binary_iarchive &, keya>::value));
    BOOST_CHECK((!key_has_boost_load<const boost::archive::binary_iarchive &, keya>::value));
    BOOST_CHECK((!key_has_boost_save<boost::archive::binary_oarchive, keyb>::value));
    BOOST_CHECK((!key_has_boost_save<boost::archive::binary_oarchive &, keyb>::value));
    BOOST_CHECK((!key_has_boost_save<boost::archive::binary_iarchive &, keyb>::value));
    BOOST_CHECK((!key_has_boost_save<boost::archive::binary_iarchive &, keyb &>::value));
    BOOST_CHECK((!key_has_boost_save<boost::archive::binary_iarchive &, const keyb &>::value));
    BOOST_CHECK((!key_has_boost_save<boost::archive::binary_iarchive &, const keyb>::value));
    BOOST_CHECK((!key_has_boost_save<const boost::archive::binary_oarchive, keyb>::value));
    BOOST_CHECK((!key_has_boost_save<const boost::archive::binary_oarchive &, keyb>::value));
    BOOST_CHECK((!key_has_boost_save<const boost::archive::binary_oarchive &, keyb>::value));
    BOOST_CHECK((!key_has_boost_load<boost::archive::binary_iarchive, keyb>::value));
    BOOST_CHECK((!key_has_boost_load<boost::archive::binary_iarchive &, keyb>::value));
    BOOST_CHECK((!key_has_boost_load<boost::archive::binary_oarchive &, keyb>::value));
    BOOST_CHECK((!key_has_boost_load<const boost::archive::binary_iarchive, keyb>::value));
    BOOST_CHECK((!key_has_boost_load<const boost::archive::binary_iarchive &, keyb>::value));
    BOOST_CHECK((!key_has_boost_load<const boost::archive::binary_iarchive &, keyb>::value));
}

struct boost_int_tester {
    template <typename T>
    void operator()(const T &) const
    {
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto i = 0; i < ntrials; ++i) {
                const auto tmp = dist(eng);
                auto cmp = boost_roundtrip(tmp);
                if (cmp != tmp) {
                    status.store(false);
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
    }
};

BOOST_AUTO_TEST_CASE(s11n_test_boost_int)
{
    boost::mpl::for_each<integral_types>(boost_int_tester());
}

struct boost_fp_tester {
    template <typename T>
    void operator()(const T &) const
    {
#if BOOST_VERSION < 106000
        // Serialization of fp types appears to be broken in previous
        // Boost versions for exact roundtrip.
        return;
#endif
#if defined(__MINGW32__)
        // It appears boost serialization of long double is broken on MinGW
        // at the present time.
        if (std::is_same<T,long double>::value) {
            return;
        }
#endif
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_real_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto i = 0; i < ntrials; ++i) {
                const auto tmp = dist(eng);
                auto cmp = boost_roundtrip(tmp);
                // NOTE: use a bit of tolerance here. Recent Boost versions do the roundtrip exactly
                // with the text archive, but earlier versions don't.
                if (std::abs((cmp - tmp) / cmp) > std::numeric_limits<T>::epsilon() * 10.) {
                    status.store(false);
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
    }
};

BOOST_AUTO_TEST_CASE(s11n_test_boost_float)
{
    boost::mpl::for_each<fp_types>(boost_fp_tester());
}

BOOST_AUTO_TEST_CASE(s11n_test_boost_string)
{
    std::atomic<bool> status(true);
    auto checker = [&status](int n) {
        // NOTE: the numerical values of the decimal digits are guaranteed to be
        // consecutive.
        std::uniform_int_distribution<char> dist('0', '9');
        std::uniform_int_distribution<unsigned> sdist(0u, 10u);
        std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
        auto gen = [&eng, &dist]() { return dist(eng); };
        std::array<char, 10u> achar;
        for (auto i = 0; i < ntrials; ++i) {
            const auto s = sdist(eng);
            std::generate_n(achar.begin(), s, gen);
            std::string str(achar.begin(), achar.begin() + s);
            const auto cmp = boost_roundtrip(str);
            if (cmp != str) {
                status.store(false);
            }
        }
    };
    std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
    t0.join();
    t1.join();
    t2.join();
    t3.join();
    BOOST_CHECK(status.load());
}

#if defined(PIRANHA_WITH_MSGPACK)

#include <cmath>
#include <iostream>
#include <iterator>

using msgpack::packer;
using msgpack::sbuffer;

template <typename T>
using sw = msgpack_stream_wrapper<T>;

// A struct with no msgpack support.
struct no_msgpack {
};

// A key with msgpack support.
struct key01 {
    key01() = default;
    key01(const key01 &) = default;
    key01(key01 &&) noexcept;
    key01 &operator=(const key01 &) = default;
    key01 &operator=(key01 &&) noexcept;
    key01(const symbol_set &);
    bool operator==(const key01 &) const;
    bool operator!=(const key01 &) const;
    bool is_compatible(const symbol_set &) const noexcept;
    bool is_ignorable(const symbol_set &) const noexcept;
    key01 merge_args(const symbol_set &, const symbol_set &) const;
    bool is_unitary(const symbol_set &) const;
    void print(std::ostream &, const symbol_set &) const;
    void print_tex(std::ostream &, const symbol_set &) const;
    template <typename T, typename U>
    std::vector<std::pair<std::string, key01>> t_subs(const std::string &, const T &, const U &,
                                                      const symbol_set &) const;
    void trim_identify(symbol_set &, const symbol_set &) const;
    key01 trim(const symbol_set &, const symbol_set &) const;
    template <typename Stream>
    int msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format, const symbol_set &) const;
    int msgpack_convert(const msgpack::object &, msgpack_format, const symbol_set &);
};

// A key without msgpack support.
struct key02 {
    key02() = default;
    key02(const key02 &) = default;
    key02(key02 &&) noexcept;
    key02 &operator=(const key02 &) = default;
    key02 &operator=(key02 &&) noexcept;
    key02(const symbol_set &);
    bool operator==(const key02 &) const;
    bool operator!=(const key02 &) const;
    bool is_compatible(const symbol_set &) const noexcept;
    bool is_ignorable(const symbol_set &) const noexcept;
    key02 merge_args(const symbol_set &, const symbol_set &) const;
    bool is_unitary(const symbol_set &) const;
    void print(std::ostream &, const symbol_set &) const;
    void print_tex(std::ostream &, const symbol_set &) const;
    template <typename T, typename U>
    std::vector<std::pair<std::string, key02>> t_subs(const std::string &, const T &, const U &,
                                                      const symbol_set &) const;
    void trim_identify(symbol_set &, const symbol_set &) const;
    key02 trim(const symbol_set &, const symbol_set &) const;
    template <typename Stream>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format, const symbol_set &);
    template <typename Stream>
    void msgpack_pack(msgpack::packer<Stream> &packer, msgpack_format) const;
    int msgpack_convert(msgpack::object &, msgpack_format, const symbol_set &);
    int msgpack_convert(const msgpack::object &, msgpack_format);
};

namespace std
{

template <>
struct hash<key01> {
    std::size_t operator()(const key01 &) const;
};

template <>
struct hash<key02> {
    std::size_t operator()(const key02 &) const;
};
}

// Helper function to roundtrip the conversion to/from msgpack for type T.
template <typename T>
static inline T msgpack_roundtrip(const T &x, msgpack_format f)
{
    sbuffer sbuf;
    packer<sbuffer> p(sbuf);
    msgpack_pack(p, x, f);
    std::size_t offset = 0u;
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size(), offset);
    piranha_assert(offset == sbuf.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    return retval;
}

// As above, but using stringstream as backend.
template <typename T>
static inline T msgpack_roundtrip_sstream(const T &x, msgpack_format f)
{
    sw<std::stringstream> oss;
    packer<sw<std::stringstream>> p(oss);
    msgpack_pack(p, x, f);
    std::vector<char> vec;
    std::copy(std::istreambuf_iterator<char>(oss), std::istreambuf_iterator<char>(), std::back_inserter(vec));
    std::size_t offset = 0u;
    auto oh = msgpack::unpack(vec.data(), vec.size(), offset);
    piranha_assert(offset == vec.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    return retval;
}

BOOST_AUTO_TEST_CASE(s11n_test_msgpack_tt)
{
    BOOST_CHECK(is_msgpack_stream<std::ostringstream>::value);
    BOOST_CHECK(!is_msgpack_stream<std::ostringstream &>::value);
    BOOST_CHECK(!is_msgpack_stream<void>::value);
    BOOST_CHECK(!is_msgpack_stream<const std::ostringstream &>::value);
    BOOST_CHECK(!is_msgpack_stream<const std::ostringstream>::value);
    BOOST_CHECK(is_msgpack_stream<sbuffer>::value);
    BOOST_CHECK(!is_msgpack_stream<float>::value);
    BOOST_CHECK(!is_msgpack_stream<const double>::value);
    BOOST_CHECK(is_msgpack_stream<sw<std::ostringstream>>::value);
    BOOST_CHECK(!is_msgpack_stream<sw<std::ostringstream> &>::value);
    BOOST_CHECK((has_msgpack_pack<sbuffer, int>::value));
    BOOST_CHECK((!has_msgpack_pack<sbuffer, void>::value));
    BOOST_CHECK((!has_msgpack_pack<void, void>::value));
    BOOST_CHECK((!has_msgpack_pack<void, int>::value));
    BOOST_CHECK((!has_msgpack_pack<sbuffer, no_msgpack>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, int>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, bool>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, bool &>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, const bool &>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, const bool>::value));
    BOOST_CHECK((!has_msgpack_pack<std::ostringstream &, const bool>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, std::string>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, std::string &>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, const std::string &>::value));
    BOOST_CHECK((has_msgpack_pack<std::ostringstream, const std::string>::value));
    BOOST_CHECK((has_msgpack_pack<sw<std::ostringstream>, int>::value));
    BOOST_CHECK((!has_msgpack_pack<sbuffer &, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream &, int>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::ostringstream &&, int>::value));
    BOOST_CHECK((!has_msgpack_pack<std::ostringstream &&, int>::value));
    BOOST_CHECK((has_msgpack_convert<int>::value));
    BOOST_CHECK((!has_msgpack_convert<void>::value));
    BOOST_CHECK((has_msgpack_convert<bool>::value));
    BOOST_CHECK((has_msgpack_convert<bool &>::value));
    BOOST_CHECK((!has_msgpack_convert<const bool>::value));
    BOOST_CHECK((has_msgpack_convert<double>::value));
    BOOST_CHECK((has_msgpack_convert<int &>::value));
    BOOST_CHECK((has_msgpack_convert<double &>::value));
    BOOST_CHECK((!has_msgpack_convert<no_msgpack>::value));
    BOOST_CHECK((!has_msgpack_convert<const int>::value));
    BOOST_CHECK((!has_msgpack_convert<const double>::value));
    BOOST_CHECK((has_msgpack_convert<int &&>::value));
    BOOST_CHECK((has_msgpack_convert<std::string>::value));
    BOOST_CHECK((has_msgpack_convert<std::string &>::value));
    BOOST_CHECK((has_msgpack_convert<std::string &&>::value));
    BOOST_CHECK((!has_msgpack_convert<const std::string>::value));
    BOOST_CHECK((!has_msgpack_convert<const std::string &>::value));
    BOOST_CHECK((has_msgpack_convert<double &&>::value));
    BOOST_CHECK((!has_msgpack_convert<const int &&>::value));
    BOOST_CHECK((!has_msgpack_convert<const double &&>::value));
    BOOST_CHECK(is_key<key01>::value);
    BOOST_CHECK((key_has_msgpack_pack<sbuffer, key01>::value));
    BOOST_CHECK((!key_has_msgpack_pack<void, key01>::value));
    BOOST_CHECK((key_has_msgpack_pack<sbuffer, key01 &>::value));
    BOOST_CHECK((key_has_msgpack_pack<sbuffer, const key01 &>::value));
    BOOST_CHECK((key_has_msgpack_pack<sbuffer, const key01>::value));
    BOOST_CHECK((!key_has_msgpack_pack<sbuffer &, key01>::value));
    BOOST_CHECK((!key_has_msgpack_pack<const sbuffer, key01>::value));
    BOOST_CHECK(is_key<key02>::value);
    BOOST_CHECK((!key_has_msgpack_pack<sbuffer, key02>::value));
    BOOST_CHECK((!key_has_msgpack_convert<key02>::value));
    BOOST_CHECK((key_has_msgpack_convert<key01>::value));
    BOOST_CHECK((key_has_msgpack_convert<key01 &>::value));
    BOOST_CHECK((!key_has_msgpack_convert<const key01 &>::value));
    BOOST_CHECK((!key_has_msgpack_convert<const key01>::value));
}

struct int_tester {
    template <typename T>
    void operator()(const T &) const
    {
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto f : {0, 1}) {
                for (auto i = 0; i < ntrials; ++i) {
                    const auto tmp = dist(eng);
                    auto cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                    cmp = msgpack_roundtrip_sstream(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
    }
};

BOOST_AUTO_TEST_CASE(s11n_test_msgpack_int)
{
    boost::mpl::for_each<integral_types>(int_tester());
    // Test bool as well.
    for (auto f : {0, 1}) {
        BOOST_CHECK_EQUAL(true, msgpack_roundtrip(true, static_cast<msgpack_format>(f)));
        BOOST_CHECK_EQUAL(false, msgpack_roundtrip(false, static_cast<msgpack_format>(f)));
    }
}

struct fp_tester {
    template <typename T>
    void operator()(const T &) const
    {
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_real_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto f : {0, 1}) {
                for (auto i = 0; i < ntrials; ++i) {
                    const auto tmp = dist(eng);
                    auto cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                    cmp = msgpack_roundtrip_sstream(tmp, static_cast<msgpack_format>(f));
                    if (cmp != tmp) {
                        status.store(false);
                    }
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
        // Additional checking for non-finite values.
        if (std::numeric_limits<T>::has_quiet_NaN && std::numeric_limits<T>::has_infinity) {
            for (auto f : {0, 1}) {
                auto tmp = std::numeric_limits<T>::quiet_NaN();
                tmp = std::copysign(tmp, T(1.));
                auto cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isnan(cmp));
                BOOST_CHECK(!std::signbit(cmp));
                tmp = std::copysign(tmp, T(-1.));
                cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isnan(cmp));
                BOOST_CHECK(std::signbit(cmp));
                tmp = std::numeric_limits<T>::infinity();
                cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isinf(cmp));
                BOOST_CHECK(!std::signbit(cmp));
                tmp = std::numeric_limits<T>::infinity();
                tmp = std::copysign(tmp, T(-1.));
                cmp = msgpack_roundtrip(tmp, static_cast<msgpack_format>(f));
                BOOST_CHECK(std::isinf(cmp));
                BOOST_CHECK(std::signbit(cmp));
            }
        }
        if (std::is_same<T, long double>::value) {
            // Check that a malformed string in the portable serialization of long double
            // raises the appropriate exception.
            sbuffer sbuf;
            packer<sbuffer> p(sbuf);
            p.pack("hello world");
            std::size_t offset = 0;
            auto oh = msgpack::unpack(sbuf.data(), sbuf.size(), offset);
            long double tmp;
            auto msg_checker = [](const std::invalid_argument &ia) -> bool {
                return boost::contains(ia.what(), "failed to parse the string 'hello world' as a long double");
            };
            BOOST_CHECK_EXCEPTION(msgpack_convert(tmp, oh.get(), msgpack_format::portable), std::invalid_argument,
                                  msg_checker);
        }
    }
};

BOOST_AUTO_TEST_CASE(s11n_test_msgpack_float)
{
    boost::mpl::for_each<fp_types>(fp_tester());
}

BOOST_AUTO_TEST_CASE(s11n_test_msgpack_string)
{
    std::atomic<bool> status(true);
    auto checker = [&status](int n) {
        std::uniform_int_distribution<char> dist('0', '9');
        std::uniform_int_distribution<unsigned> sdist(0u, 10u);
        std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
        auto gen = [&eng, &dist]() { return dist(eng); };
        std::array<char, 10u> achar;
        for (auto i = 0; i < ntrials; ++i) {
            for (msgpack_format f : {msgpack_format::portable, msgpack_format::binary}) {
                const auto s = sdist(eng);
                std::generate_n(achar.begin(), s, gen);
                std::string str(achar.begin(), achar.begin() + s);
                const auto cmp = msgpack_roundtrip(str, f);
                if (cmp != str) {
                    status.store(false);
                }
            }
        }
    };
    std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
    t0.join();
    t1.join();
    t2.join();
    t3.join();
    BOOST_CHECK(status.load());
}

#endif

static const int ntrials_file = 20;

static const std::vector<data_format> dfs = {data_format::boost_binary, data_format::boost_portable,
                                             data_format::msgpack_binary, data_format::msgpack_portable};

static const std::vector<compression> cfs
    = {compression::none, compression::bzip2, compression::zlib, compression::gzip};

template <typename T>
static inline T save_roundtrip(const T &x, data_format f, compression c)
{
    tmp_file file;
    save_file(x, file.name(), f, c);
    T retval;
    load_file(retval, file.name(), f, c);
    return retval;
}

struct int_save_load_tester {
    template <typename T>
    void operator()(const T &) const
    {
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto i = 0; i < ntrials_file; ++i) {
                for (auto f : dfs) {
                    for (auto c : cfs) {
                        const auto tmp = dist(eng);
#if defined(PIRANHA_WITH_MSGPACK) && defined(PIRANHA_WITH_ZLIB)
                        // NOTE: we are not expecting any failure if we have all optional deps.
                        auto cmp = save_roundtrip(tmp, f, c);
                        if (cmp != tmp) {
                            status.store(false);
                        }
#else
                        // If msgpack or zlib are not available, we will have not_implemented_error
                        // failures.
                        try {
                            auto cmp = save_roundtrip(tmp, f, c);
                            if (cmp != tmp) {
                                status.store(false);
                            }
                        } catch (const not_implemented_error &) {
                            continue;
                        }
#endif
                    }
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
    }
};

struct fp_save_load_tester {
    template <typename T>
    void operator()(const T &) const
    {
#if BOOST_VERSION < 106000
        // Serialization of fp types appears to be broken in previous
        // Boost versions for exact roundtrip.
        return;
#endif
#if defined(__MINGW32__)
        // It appears boost serialization of long double is broken on MinGW
        // at the present time.
        if (std::is_same<T,long double>::value) {
            return;
        }
#endif
        std::atomic<bool> status(true);
        auto checker = [&status](int n) {
            std::uniform_real_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
            std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
            for (auto i = 0; i < ntrials_file; ++i) {
                for (auto f : dfs) {
                    for (auto c : cfs) {
                        const auto tmp = dist(eng);
#if defined(PIRANHA_WITH_MSGPACK) && defined(PIRANHA_WITH_ZLIB)
                        auto cmp = save_roundtrip(tmp, f, c);
                        if (cmp != tmp) {
                            status.store(false);
                        }
#else
                        try {
                            auto cmp = save_roundtrip(tmp, f, c);
                            if (cmp != tmp) {
                                status.store(false);
                            }
                        } catch (const not_implemented_error &) {
                            continue;
                        }
#endif
                    }
                }
            }
        };
        std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
        t0.join();
        t1.join();
        t2.join();
        t3.join();
        BOOST_CHECK(status.load());
    }
};

struct no_boost_msgpack {
};

struct only_boost {
};

namespace piranha
{

template <typename Archive>
class boost_save_impl<Archive, only_boost>
{
public:
    void operator()(Archive &, const only_boost &) const
    {
    }
};

template <typename Archive>
class boost_load_impl<Archive, only_boost>
{
public:
    void operator()(Archive &, only_boost &) const
    {
    }
};
}

// Save/load checker for string.
static inline void string_save_load_tester()
{
    std::atomic<bool> status(true);
    auto checker = [&status](int n) {
        // NOTE: the numerical values of the decimal digits are guaranteed to be
        // consecutive.
        std::uniform_int_distribution<char> dist('0', '9');
        std::uniform_int_distribution<unsigned> sdist(0u, 10u);
        std::mt19937 eng(static_cast<std::mt19937::result_type>(n));
        auto gen = [&eng, &dist]() { return dist(eng); };
        std::array<char, 10u> achar;
        for (auto i = 0; i < ntrials_file; ++i) {
            for (auto f : dfs) {
                for (auto c : cfs) {
                    const auto s = sdist(eng);
                    std::generate_n(achar.begin(), s, gen);
                    std::string str(achar.begin(), achar.begin() + s);
#if defined(PIRANHA_WITH_MSGPACK) && defined(PIRANHA_WITH_ZLIB)
                    // NOTE: we are not expecting any failure if we have all optional deps.
                    auto cmp = save_roundtrip(str, f, c);
                    if (cmp != str) {
                        status.store(false);
                    }
#else
                    // If msgpack or zlib are not available, we will have not_implemented_error
                    // failures.
                    try {
                        auto cmp = save_roundtrip(str, f, c);
                        if (cmp != str) {
                            status.store(false);
                        }
                    } catch (const not_implemented_error &) {
                        continue;
                    }
#endif
                }
            }
        }
    };
    std::thread t0(checker, 0), t1(checker, 1), t2(checker, 2), t3(checker, 3);
    t0.join();
    t1.join();
    t2.join();
    t3.join();
    BOOST_CHECK(status.load());
}

BOOST_AUTO_TEST_CASE(s11n_test_save_load)
{
    boost::mpl::for_each<integral_types>(int_save_load_tester());
    boost::mpl::for_each<fp_types>(fp_save_load_tester());
    string_save_load_tester();
#if defined(PIRANHA_WITH_MSGPACK) && defined(PIRANHA_WITH_ZLIB)
    // Test failures.
    for (auto f : dfs) {
        for (auto c : cfs) {
            // Unserializable type.
            no_boost_msgpack n;
            auto msg_checker = [](const not_implemented_error &nie) -> bool {
                return boost::contains(nie.what(),
                                       "type '" + detail::demangle<no_boost_msgpack>() + "' does not support");
            };
            BOOST_CHECK_EXCEPTION(save_file(n, "foo", f, c), not_implemented_error, msg_checker);
            BOOST_CHECK_EXCEPTION(load_file(n, "foo", f, c), not_implemented_error, msg_checker);
            // Wrong filename for loading.
            auto msg_checker2 = [](const std::runtime_error &re) -> bool {
                return boost::contains(re.what(), "file 'foobar123' could not be opened for loading");
            };
            int m = 0;
            BOOST_CHECK_EXCEPTION(load_file(m, "foobar123", f, c), std::runtime_error, msg_checker2);
        }
    }
    BOOST_CHECK((has_boost_save<boost::archive::binary_oarchive, only_boost>::value));
    BOOST_CHECK((has_boost_load<boost::archive::binary_iarchive, only_boost>::value));
    BOOST_CHECK_NO_THROW(save_roundtrip(only_boost{}, data_format::boost_portable, compression::none));
    BOOST_CHECK_NO_THROW(save_roundtrip(only_boost{}, data_format::boost_binary, compression::none));
    BOOST_CHECK_THROW(save_roundtrip(only_boost{}, data_format::msgpack_portable, compression::none),
                      not_implemented_error);
    BOOST_CHECK_THROW(save_roundtrip(only_boost{}, data_format::msgpack_binary, compression::none),
                      not_implemented_error);
#endif
}
