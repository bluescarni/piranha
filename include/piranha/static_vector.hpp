/* Copyright 2009-2017 Francesco Biscani (bluescarni@gmail.com)

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

#ifndef PIRANHA_STATIC_VECTOR_HPP
#define PIRANHA_STATIC_VECTOR_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <tuple>
#include <type_traits>

#include <piranha/config.hpp>
#include <piranha/detail/init.hpp>
#include <piranha/detail/small_vector_fwd.hpp>
#include <piranha/detail/vector_hasher.hpp>
#include <piranha/exceptions.hpp>
#include <piranha/s11n.hpp>
#include <piranha/safe_cast.hpp>
#include <piranha/type_traits.hpp>

namespace piranha
{

namespace detail
{

// TMP to determine size type big enough to represent Size. The candidate types are the fundamental unsigned integer
// types.
using static_vector_size_types = std::tuple<unsigned char, unsigned short, unsigned, unsigned long, unsigned long long>;

template <std::size_t Size, std::size_t Index = 0u>
struct static_vector_size_type {
    using candidate_type = typename std::tuple_element<Index, static_vector_size_types>::type;
    using type = typename std::conditional<
        (std::numeric_limits<candidate_type>::max() >= Size), candidate_type,
        typename static_vector_size_type<Size, static_cast<std::size_t>(Index + 1u)>::type>::type;
};

template <std::size_t Size>
struct static_vector_size_type<Size, static_cast<std::size_t>(std::tuple_size<static_vector_size_types>::value - 1u)> {
    using type =
        typename std::tuple_element<static_cast<std::size_t>(std::tuple_size<static_vector_size_types>::value - 1u),
                                    static_vector_size_types>::type;
    static_assert(std::numeric_limits<type>::max() >= Size, "Cannot determine size type for static vector.");
};
}

/// Static vector class.
/**
 * This class represents a dynamic array that avoids runtime memory allocation. Memory storage is provided by
 * a contiguous memory block stored within the class. The size of the internal storage is enough to fit
 * at least \p MaxSize objects.
 *
 * The interface of this class mimics part of the interface of \p std::vector.
 *
 * ## Type requirements ##
 *
 * - \p T must satisfy piranha::is_container_element.
 * - \p MaxSize must be non-null.
 *
 * ## Exception safety guarantee ##
 *
 * Unless otherwise specified, this class provides the strong exception safety guarantee for all operations.
 *
 * ## Move semantics ##
 *
 * After a move operation, the container will be left in a state equivalent to a default-constructed instance.
 */
template <typename T, std::size_t MaxSize>
class static_vector
{
    static_assert(MaxSize > 0u, "Maximum size must be strictly positive.");
    template <typename, typename>
    friend union detail::small_vector_union;

public:
    /// Size type.
    /**
     * An unsigned integer type large enough to represent \p MaxSize.
     */
    using size_type = typename detail::static_vector_size_type<MaxSize>::type;

private:
    PIRANHA_TT_CHECK(is_container_element, T);
    // This check is against overflows when using memcpy.
    static_assert(std::numeric_limits<size_type>::max() <= std::numeric_limits<std::size_t>::max() / sizeof(T),
                  "The size type for static_vector might overflow.");
    // Check for overflow in the definition of the storage type.
    static_assert(MaxSize < std::numeric_limits<std::size_t>::max() / sizeof(T),
                  "Overflow in the computation of storage size.");
    // NOTE: some notes about the use of raw storage, after some research in the standard:
    // - storage type is guaranteed to be a POD able to hold any object whose size is at most Len
    //   and alignment a divisor of Align (20.9.7.6). Thus, we can store on object of type T at the
    //   beginning of the storage;
    // - POD types are trivially copyable, and thus they occupy contiguous bytes of storage (1.8/5);
    // - the meaning of "contiguous" seems not to be explicitly stated, but it can be inferred
    //   (e.g., 23.3.2.1/1) that it has to do with the fact that one can do pointer arithmetics
    //   to move around in the storage;
    // - the footnote in 5.7/6 implies that casting around to different pointer types and then doing
    //   pointer arithmetics has the expected behaviour of moving about with discrete offsets equal to the sizeof
    //   the pointed-to object (i.e., it seems like pointer arithmetics does not suffer from
    //   strict aliasing and type punning issues - at least as long as one goest through void *
    //   conversions to get the address of the storage - note also 3.9.2/3). This supposedly applies to arrays only,
    //   but, again, it is implied in other places that it also holds true for contiguous storage;
    // - the no-padding guarantee for arrays (which also consist of contiguous storage) implies that sizeof must be
    //   a multiple of the alignment for a given type;
    // - the definition of alignment (3.11) implies that if one has contiguous storage starting at an address in
    //   which T can be constructed, the offsetting the initial address by multiples of the alignment value will
    //   still produce addresses at which the object can be constructed;
    // - in general, we are assuming here that we can handle contiguous storage the same way arrays can be handled
    //   (e.g., to get the end() iterator we get one past the last element);
    // - note that placement new will work as expected (i.e., it will construct the object exactly at the address passed
    //   in as parameter).
    using storage_type = typename std::aligned_storage<sizeof(T) * MaxSize, alignof(T)>::type;
#if defined(PIRANHA_WITH_BOOST_S11N)
    // Boost serialization support.
    friend class boost::serialization::access;
    template <class Archive>
    void save(Archive &ar, unsigned) const
    {
        boost_save_vector(ar, *this);
    }
    template <class Archive>
    void load(Archive &ar, unsigned)
    {
        boost_load_vector(ar, *this);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif
    // This is a small helper to default-init a range via positional new.
    // We use this in various POD optimisations below in order to make sure
    // objects of type T exist in the raw storage before doing anything with them.
    static void default_init(T *begin, T *end)
    {
        for (; begin != end; ++begin) {
            ::new (static_cast<void *>(begin)) T;
        }
    }

public:
    /// Maximum size.
    /**
     * Alias for \p MaxSize.
     */
    static const size_type max_size = MaxSize;
    /// Contained type.
    typedef T value_type;
    /// Iterator type.
    typedef value_type *iterator;
    /// Const iterator type.
    typedef value_type const *const_iterator;
    /// Default constructor.
    /**
     * Will construct a vector of size 0.
     */
    static_vector() : m_tag(1u), m_size(0u) {}
    /// Copy constructor.
    /**
     * @param other target of the copy operation.
     *
     * @throws unspecified any exception thrown by the copy constructor of \p T.
     */
    static_vector(const static_vector &other) : m_tag(1u), m_size(0u)
    {
        const auto size = other.size();
        // NOTE: here and elsewhere, the standard implies (3.9/2) that we can use this optimisation
        // for trivially copyable types. GCC does not support the type trait yet, so we restrict the
        // optimisation to POD types (which are trivially copyable).
        if (std::is_pod<T>::value) {
            // NOTE: we need to def init objects of type T inside the buffer. Unlike in C, objects with trivial default
            // constructors cannot be created by simply reinterpreting suitably aligned storage,
            // such as memory allocated with std::malloc: placement-new is required to formally introduce a new objects
            // and avoid potential undefined behaviour. This is likely to be optimised away by the compiler.
            default_init(ptr(), ptr() + size);
            std::memcpy(vs(), other.vs(), size * sizeof(T));
            m_size = size;
        } else {
            try {
                for (size_type i = 0u; i < size; ++i) {
                    push_back(other[i]);
                }
            } catch (...) {
                // Roll back the constructed items before re-throwing.
                destroy_items();
                throw;
            }
        }
    }
    /// Move constructor.
    /**
     * @param other target of the move operation.
     */
    static_vector(static_vector &&other) noexcept : m_tag(1u), m_size(0u)
    {
        const auto size = other.size();
        if (std::is_pod<T>::value) {
            default_init(ptr(), ptr() + size);
            std::memcpy(vs(), other.vs(), size * sizeof(T));
            m_size = size;
        } else {
            // NOTE: here no need for rollback, as we assume move ctors do not throw.
            for (size_type i = 0u; i < size; ++i) {
                push_back(std::move(other[i]));
            }
        }
        // Nuke other.
        other.destroy_items();
        other.m_size = 0u;
    }
    /// Constructor from multiple copies.
    /**
     * Will construct a vector containing \p n copies of \p x.
     *
     * @param n number of copies of \p x that will be inserted in the vector.
     * @param x element whose copies will be inserted in the vector.
     *
     * @throws unspecified any exception thrown by push_back().
     */
    explicit static_vector(const size_type &n, const value_type &x) : m_tag(1u), m_size(0u)
    {
        try {
            for (size_type i = 0u; i < n; ++i) {
                push_back(x);
            }
        } catch (...) {
            destroy_items();
            throw;
        }
    }
    /// Destructor.
    /**
     * Will destroy all elements of the vector.
     */
    ~static_vector()
    {
        // NOTE: here we should replace with bidirectional tt, if we ever implement it.
        PIRANHA_TT_CHECK(is_forward_iterator, iterator);
        PIRANHA_TT_CHECK(is_forward_iterator, const_iterator);
        piranha_assert(m_tag == 1u);
        destroy_items();
    }
    /// Copy assignment operator.
    /**
     * @param other target of the copy assignment operation.
     *
     * @return reference to \p this.
     *
     * @throws unspecified any exception thrown by the copy constructor of \p T.
     */
    static_vector &operator=(const static_vector &other)
    {
        if (likely(this != &other)) {
            if (std::is_pod<T>::value) {
                if (other.m_size > m_size) {
                    // If other is larger, we need to make sure we have created the excess objects
                    // before writing into them.
                    default_init(ptr() + m_size, ptr() + other.m_size);
                }
                std::memcpy(vs(), other.vs(), other.m_size * sizeof(T));
                m_size = other.m_size;
            } else {
                static_vector tmp(other);
                *this = std::move(tmp);
            }
        }
        return *this;
    }
    /// Move assignment operator.
    /**
     * @param other target of the move assignment operation.
     *
     * @return reference to \p this.
     */
    static_vector &operator=(static_vector &&other) noexcept
    {
        if (likely(this != &other)) {
            if (std::is_pod<T>::value) {
                if (other.m_size > m_size) {
                    default_init(ptr() + m_size, ptr() + other.m_size);
                }
                std::memcpy(vs(), other.vs(), other.m_size * sizeof(T));
            } else {
                const size_type old_size = m_size, new_size = other.m_size;
                if (old_size <= new_size) {
                    // Move assign new content into old content.
                    for (size_type i = 0u; i < old_size; ++i) {
                        (*this)[i] = std::move(other[i]);
                    }
                    // Move construct remaining elements.
                    for (size_type i = old_size; i < new_size; ++i) {
                        ::new (static_cast<void *>(ptr() + i)) value_type(std::move(other[i]));
                    }
                } else {
                    // Move assign new content into old content.
                    for (size_type i = 0u; i < new_size; ++i) {
                        (*this)[i] = std::move(other[i]);
                    }
                    // Destroy excess content.
                    for (size_type i = new_size; i < old_size; ++i) {
                        ptr()[i].~T();
                    }
                }
            }
            m_size = other.m_size;
            // Nuke the other.
            other.destroy_items();
            other.m_size = 0u;
        }
        return *this;
    }
    /// Const subscript operator.
    /**
     * @param n index of the desired element.
     *
     * @return const reference to the element contained at index \p n;
     */
    const value_type &operator[](const size_type &n) const
    {
        return ptr()[n];
    }
    /// Subscript operator.
    /**
     * @param n index of the desired element.
     *
     * @return reference to the element contained at index \p n;
     */
    value_type &operator[](const size_type &n)
    {
        return ptr()[n];
    }
    /// Begin iterator.
    /**
     * @return iterator to the first element, or end() if the vector is empty.
     */
    iterator begin()
    {
        return ptr();
    }
    /// End iterator.
    /**
     * @return iterator to the position one past the last element of the vector.
     */
    iterator end()
    {
        return ptr() + m_size;
    }
    /// Const begin iterator.
    /**
     * @return const iterator to the first element, or end() if the vector is empty.
     */
    const_iterator begin() const
    {
        return ptr();
    }
    /// Const end iterator.
    /**
     * @return const iterator to the position one past the last element of the vector.
     */
    const_iterator end() const
    {
        return ptr() + m_size;
    }
    /// Equality operator.
    /**
     * @param other argument for the comparison.
     *
     * @return \p true if the sizes of the vectors are the same and all elements of \p this compare as equal
     * to the elements in \p other, \p false otherwise.
     *
     * @throws unspecified any exception thrown by the comparison operator of the value type.
     */
    bool operator==(const static_vector &other) const
    {
        return (m_size == other.m_size && std::equal(begin(), end(), other.begin()));
    }
    /// Inequality operator.
    /**
     * @param other argument for the comparison.
     *
     * @return the opposite of operator==().
     *
     * @throws unspecified any exception thrown by operator==().
     */
    bool operator!=(const static_vector &other) const
    {
        return !operator==(other);
    }
    /// Copy-add element at the end of the vector.
    /**
     * \p x is copy-inserted at the end of the container.
     *
     * @param x element to be inserted.
     *
     * @throws std::bad_alloc if the insertion of \p x would lead to a size greater than \p MaxSize.
     * @throws unspecified any exception thrown by the copy constructor of \p T.
     */
    void push_back(const value_type &x)
    {
        if (unlikely(m_size == MaxSize)) {
            piranha_throw(std::bad_alloc, );
        }
        ::new (static_cast<void *>(ptr() + m_size)) value_type(x);
        ++m_size;
    }
    /// Move-add element at the end of the vector.
    /**
     * \p x is move-inserted at the end of the container.
     *
     * @param x element to be inserted.
     *
     * @throws std::bad_alloc if the insertion of \p x would lead to a size greater than \p MaxSize.
     */
    void push_back(value_type &&x)
    {
        if (unlikely(m_size == MaxSize)) {
            piranha_throw(std::bad_alloc, );
        }
        ::new (static_cast<void *>(ptr() + m_size)) value_type(std::move(x));
        ++m_size;
    }

private:
    template <typename... Args>
    using emplace_enabler = enable_if_t<std::is_constructible<value_type, Args &&...>::value, int>;

public:
    /// Construct in-place at the end of the vector.
    /**
     * \note
     * This method is enabled only if \p value_type is constructible from the variadic arguments pack.
     *
     * Input parameters will be used to construct an instance of \p T at the end of the container.
     *
     * @param params arguments that will be used to construct the new element.
     *
     * @throws std::bad_alloc if the insertion of the new element would lead to a size greater than \p MaxSize.
     * @throws unspecified any exception thrown by the constructor of \p T from the input parameters.
     */
    template <typename... Args, emplace_enabler<Args...> = 0>
    void emplace_back(Args &&... params)
    {
        if (unlikely(m_size == MaxSize)) {
            piranha_throw(std::bad_alloc, );
        }
        ::new (static_cast<void *>(ptr() + m_size)) value_type(std::forward<Args>(params)...);
        ++m_size;
    }
    /// Size.
    /**
     * @return the number of elements currently stored in \p this.
     */
    size_type size() const
    {
        return m_size;
    }
    /// Empty test.
    /**
     * @return \p true if the size of the container is zero, \p false otherwise.
     */
    bool empty() const
    {
        return m_size == 0u;
    }
    /// Resize.
    /**
     * After this operation, the number of elements stored in the container will be \p new_size. If \p new_size is
     * greater than the size of the object before the operation, the new elements will be value-initialized and placed
     * at the end of the container. If \p new_size is smaller than the size of the object before the operation, the
     * first \p new_size object in the vector will be preserved.
     *
     * @param new_size new size for the vector.
     *
     * @throws std::bad_alloc if \p new_size is greater than \p MaxSize.
     * @throws unspecified any exception thrown by the default constructor of \p T.
     */
    void resize(const size_type &new_size)
    {
        if (unlikely(new_size > MaxSize)) {
            piranha_throw(std::bad_alloc, );
        }
        const auto old_size = m_size;
        if (new_size == old_size) {
            return;
        } else if (new_size > old_size) {
            size_type i = old_size;
            // Construct new in case of larger size.
            try {
                for (; i < new_size; ++i) {
                    // NOTE: placement-new syntax for value initialization, the same as performed by
                    // std::vector (see 23.3.6.2 and 23.3.6.3/9).
                    // http://en.cppreference.com/w/cpp/language/value_initialization
                    ::new (static_cast<void *>(ptr() + i)) value_type();
                    piranha_assert(m_size != MaxSize);
                    ++m_size;
                }
            } catch (...) {
                for (size_type j = old_size; j < i; ++j) {
                    ptr()[j].~T();
                    piranha_assert(m_size);
                    --m_size;
                }
                throw;
            }
        } else {
            // Destroy in case of smaller size.
            for (size_type i = new_size; i < old_size; ++i) {
                ptr()[i].~T();
                piranha_assert(m_size);
                --m_size;
            }
        }
    }
    /// Erase element.
    /**
     * This method will erase the element to which \p it points. The return value will be the iterator
     * following the erased element (which will be the end iterator if \p it points to the last element
     * of the vector).
     *
     * \p it must be a valid iterator to an element in \p this.
     *
     * @param it iterator to the element of \p this to be removed.
     *
     * @return the iterator following the erased element.
     */
    iterator erase(const_iterator it)
    {
        // NOTE: on the use of const_cast: the object 'it' points to is *not* const by definition
        // (it is an element of a vector). So the const casting should be ok. See these links:
        // http://stackoverflow.com/questions/30770944/is-this-a-valid-usage-of-const-cast
        // http://en.cppreference.com/w/cpp/language/const_cast
        // All of this is noexcept.
        piranha_assert(it != end());
        // The return value is the original iterator, which will eventually contain the value
        // following the original 'it'. If 'it' pointed to the last element, then retval
        // will be the new end().
        auto retval = const_cast<iterator>(it);
        const auto it_f = end() - 1;
        // NOTE: POD optimisation is possible here, but requires pointer diff.
        for (; it != it_f; ++it) {
            // Destroy the current element.
            it->~T();
            // Move-init the current iterator content with the next element.
            ::new (static_cast<void *>(const_cast<iterator>(it))) value_type(std::move(*(it + 1)));
        }
        // Destroy the last element.
        it->~T();
        // Update the size.
        m_size = static_cast<size_type>(m_size - 1u);
        return retval;
    }
    /// Clear.
    /**
     * This method will destroy all elements of the vector and set the size to zero.
     */
    void clear()
    {
        destroy_items();
        m_size = 0u;
    }

private:
    template <typename U>
    using hash_enabler = enable_if_t<is_hashable<U>::value, int>;

public:
    /// Hash value.
    /**
     * \note
     * This method is enabled only if \p T satisfies piranha::is_hashable.
     *
     * @return one of the following:
     * - 0 if size() is 0,
     * - the hash of the first element (via \p std::hash) if size() is 1,
     * - in all other cases, the result of iteratively mixing via \p boost::hash_combine the hash
     *   values of all the elements of the container, calculated via \p std::hash,
     *   with the hash value of the first element as seed value.
     *
     * @throws unspecified any exception resulting from calculating the hash value(s) of the
     * individual elements.
     *
     * @see http://www.boost.org/doc/libs/release/doc/html/hash/combine.html
     */
    template <typename U = T, hash_enabler<U> = 0>
    std::size_t hash() const
    {
        return detail::vector_hasher(*this);
    }

private:
    template <typename U>
    using ostream_enabler = enable_if_t<is_ostreamable<U>::value, int>;

public:
    /// Stream operator overload for piranha::static_vector.
    /**
     * \note
     * This function is enabled only if \p U satisfies piranha::is_ostreamable.
     *
     * Will print to stream a human-readable representation of \p v.
     *
     * @param os target stream.
     * @param v vector to be streamed.
     *
     * @return reference to \p os.
     *
     * @throws unspecified any exception thrown by printing to stream instances of the value type of \p v.
     */
    template <typename U = T, ostream_enabler<U> = 0>
    friend std::ostream &operator<<(std::ostream &os, const static_vector &v)
    {
        os << '[';
        for (decltype(v.size()) i = 0u; i < v.size(); ++i) {
            os << v[i];
            if (i != v.size() - 1u) {
                os << ',';
            }
        }
        os << ']';
        return os;
    }

private:
    void destroy_items()
    {
        // NOTE: no need to destroy anything in this case:
        // http://en.cppreference.com/w/cpp/language/destructor
        // NOTE: here we could have the destructor defined in a base class to be specialised
        // if T is trivially destructible. Then we can default the dtor here and static_vector
        // will be trivially destructible if T is.
        if (std::is_trivially_destructible<T>::value) {
            return;
        }
        const auto size = m_size;
        for (size_type i = 0u; i < size; ++i) {
            ptr()[i].~T();
        }
    }
    // Getters for storage cast to void.
    void *vs()
    {
        return static_cast<void *>(&m_storage);
    }
    const void *vs() const
    {
        return static_cast<const void *>(&m_storage);
    }
    // Getters for T pointer.
    T *ptr()
    {
        return static_cast<T *>(vs());
    }
    const T *ptr() const
    {
        return static_cast<const T *>(vs());
    }

private:
    unsigned char m_tag;
    size_type m_size;
    storage_type m_storage;
};

template <typename T, std::size_t MaxSize>
const typename static_vector<T, MaxSize>::size_type static_vector<T, MaxSize>::max_size;

#if defined(PIRANHA_WITH_BOOST_S11N)

/// Specialisation of piranha::boost_save() for piranha::static_vector.
/**
 * \note
 * This specialisation is enabled only if \p T and the size type of the vector satisfy
 * piranha::has_boost_save.
 *
 * @throws unspecified any exception thrown by piranha::boost_save().
 */
template <typename Archive, typename T, std::size_t S>
struct boost_save_impl<Archive, static_vector<T, S>, boost_save_vector_enabler<Archive, static_vector<T, S>>>
    : boost_save_via_boost_api<Archive, static_vector<T, S>> {
};

/// Specialisation of piranha::boost_load() for piranha::static_vector.
/**
 * \note
 * This specialisation is enabled only if \p T and the size type of the vector satisfy
 * piranha::has_boost_load.
 *
 * The basic exception safety guarantee is provided.
 *
 * @throws unspecified any exception thrown by:
 * - piranha::boost_load(),
 * - piranha::static_vector::resize().
 */
template <typename Archive, typename T, std::size_t S>
struct boost_load_impl<Archive, static_vector<T, S>, boost_load_vector_enabler<Archive, static_vector<T, S>>>
    : boost_load_via_boost_api<Archive, static_vector<T, S>> {
};

#endif

#if defined(PIRANHA_WITH_MSGPACK)

/// Specialisation of piranha::msgpack_pack() for piranha::static_vector.
/**
 * \note
 * This specialisation is enabled only if:
 * - \p Stream satisfies piranha::is_msgpack_stream,
 * - \p T satisfies piranha::has_msgpack_pack,
 * - the size type of the vector can be safely converted to \p std::uint32_t.
 */
template <typename Stream, typename T, std::size_t S>
struct msgpack_pack_impl<Stream, static_vector<T, S>, msgpack_pack_vector_enabler<Stream, static_vector<T, S>>> {
    /// Call operator.
    /**
     * This method will serialize into \p p the input vector \p v using the format \p f.
     *
     * @param p the target <tt>msgpack::packer</tt>.
     * @param v the vector to be serialized.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::packer</tt>,
     * - piranha::safe_cast(),
     * - piranha::msgpack_pack().
     */
    void operator()(msgpack::packer<Stream> &p, const static_vector<T, S> &v, msgpack_format f) const
    {
        msgpack_pack_vector(p, v, f);
    }
};

/// Specialisation of piranha::msgpack_convert() for piranha::static_vector.
/**
 * \note
 * This specialisation is enabled only if:
 * - \p T satisfies piranha::has_msgpack_convert,
 * - the size type of \p std::vector can be safely converted to the size type of piranha::static_vector.
 */
template <typename T, std::size_t S>
struct msgpack_convert_impl<static_vector<T, S>, msgpack_convert_array_enabler<static_vector<T, S>>> {
    /// Call operator.
    /**
     * This method will convert \p o into \p v using the format \p f. This method provides the basic exception safety
     * guarantee.
     *
     * @param v the vector into which the content of \p o will deserialized.
     * @param o the source <tt>msgpack::object</tt>.
     * @param f the desired piranha::msgpack_format.
     *
     * @throws unspecified any exception thrown by:
     * - the public interface of <tt>msgpack::object</tt>,
     * - memory errors in standard containers,
     * - piranha::safe_cast(),
     * - piranha::msgpack_convert().
     */
    void operator()(static_vector<T, S> &v, const msgpack::object &o, msgpack_format f) const
    {
        msgpack_convert_array(o, v, f);
    }
};

#endif
}

#endif
