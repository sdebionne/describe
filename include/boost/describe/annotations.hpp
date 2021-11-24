// Copyright (C) 2021 Samuel Debionne, ESRF.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if !defined(BOOST_DESCRIBE_ANNOTATIONS_HPP)
#define BOOST_DESCRIBE_ANNOTATIONS_HPP

#include <type_traits>

#include <boost/describe/descriptor_by_name.hpp>
#include <boost/describe/descriptor_by_pointer.hpp>
#include <boost/mp11.hpp>
#include <boost/describe/class_of.hpp>

namespace boost
{
namespace describe
{
namespace detail
{
    template <auto N, auto P> struct ND {};

    constexpr unsigned cx_hash( char const* s )
    {
        unsigned r = 0;

        for( ; *s; ++s )
        {
            r = r * 31 + (unsigned char)*s;
        }

        return r;
    }
} // namespace detail

// Normalized descriptor
template<class D> using normalize_descriptor = detail::ND<detail::cx_hash(D::name), D::pointer>;

namespace detail
{
    template <typename A>
    struct member_annotation
    {
        static constexpr decltype(A::name()) name = A::name();
        static constexpr decltype(A::value()) value = A::value();
    };

    template <typename... T>
    auto member_annotation_fn_impl(int, T...)
    {
        return list<member_annotation<T>...>();
    }

#define BOOST_ANNOTATE_MEMBER_IMPL(C, n, v)                                                \
    , [] {                                                                                 \
        struct _boost_note                                                                 \
        {                                                                                  \
            static constexpr auto name() noexcept { return BOOST_DESCRIBE_PP_NAME(n); }    \
            static constexpr auto value() noexcept { return BOOST_DESCRIBE_PP_EXPAND(v); } \
        };                                                                                 \
        return _boost_note();                                                              \
    }()

#define BOOST_ANNOTATE_MEMBER_IMPL_(a) BOOST_ANNOTATE_MEMBER_IMPL(a)

#define BOOST_ANNOTATE_MEMBER_IMPL__(m, Annotation) BOOST_ANNOTATE_MEMBER_IMPL_(m BOOST_DESCRIBE_PP_UNPACK Annotation)

#define BOOST_DESCRIBE_NORMALIZED_MEMBER_DESCRIPTOR(C, m) \
    boost::describe::detail::ND<boost::describe::detail::cx_hash(#m), &C::m>

#define BOOST_ANNOTATE_MEMBER(C, m, ...)                                                             \
    static_assert(std::is_class_v<C>, "BOOST_ANNOTATE_MEMBER should only be used with class types"); \
    inline auto boost_annotate_fn(C*, BOOST_DESCRIBE_NORMALIZED_MEMBER_DESCRIPTOR(C, m)*)            \
    {                                                                                                \
        return boost::describe::detail::member_annotation_fn_impl(                                   \
            0 BOOST_DESCRIBE_PP_FOR_EACH(BOOST_ANNOTATE_MEMBER_IMPL__, C, ##__VA_ARGS__));           \
    }
} // namespace detail

template <class T, class Md, class Nd = boost::describe::normalize_descriptor<Md>>
using annotate_member = decltype(boost_annotate_fn(static_cast<T*>(0), static_cast<Nd*>(0)));

// Get annotation by name (requires C++20)
template <typename A, auto name, typename P = detail::by_name<name>>
using annotation_by_name = mp11::mp_at<A, mp11::mp_find_if_q<A, P>>;

} //namespace describe
} //namespace boost

#endif //!defined(BOOST_DESCRIBE_ANNOTATIONS_HPP)
