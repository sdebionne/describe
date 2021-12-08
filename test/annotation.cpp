// Copyright (C) 2018 Samuel Debionne, ESRF.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <iostream>
#include <vector>
#include <type_traits>

#include <boost/describe.hpp>
#include <boost/describe/annotations.hpp>
#include <boost/describe/is_described.hpp>
#include <boost/mp11.hpp>
#include <boost/callable_traits/return_type.hpp>

#include <lima/core/rectangle.hpp>
#include <lima/core/io.hpp>

template <typename Rep>
std::ostream& operator<<(std::ostream& os, std::chrono::duration<Rep> const& duration)
{
    os << duration.count() << "s";
    return os;
}

template <typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
std::ostream& operator<<(std::ostream& os, E const& e)
{
    char const* r = "(unnamed)";

    boost::mp11::mp_for_each<boost::describe::describe_enumerators<E>>([&](auto D) {
        if (e == D.value)
            os << D.name;
    });

    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> const& v)
{
    os << "[" << v.size() << "]";
    return os;
}

namespace lima
{
BOOST_DESCRIBE_STRUCT(rectangle<std::ptrdiff_t>, (), (topleft, dimensions))
//BOOST_DESCRIBE_STRUCT(rectangle, (), (topleft, dimensions))

// clang-format off
BOOST_ANNOTATE_MEMBER(rectangle<std::ptrdiff_t>, topleft,
    (doc, "top left corner coordinate"),
    (desc, "The top left corner coordinate of the region of interest to transfer"))

BOOST_ANNOTATE_MEMBER(rectangle<std::ptrdiff_t>, dimensions,
    (doc, "dimensions"),
    (desc, "The dimensions of the region of interest to transfer"))
// clang-format on

enum class acq_mode_enum : int
{
    normal,      //!< Single image
    accumulation //!< Multiple image accumulated (over time)
};

BOOST_DESCRIBE_ENUM(acq_mode_enum, normal, accumulation)

using roi_t = rectangle<std::ptrdiff_t>;

struct acquisition
{
    int nb_frames = 1;                                              // POD
    std::chrono::duration<int> expo_time = std::chrono::seconds(1); // Class
    acq_mode_enum acq_mode = acq_mode_enum::normal;                 // Enumerator
    roi_t roi;                                                      // Described class
    std::vector<roi_t> counters;                                    // Vector of described class
};

BOOST_DESCRIBE_STRUCT(acquisition, (), (nb_frames, expo_time, acq_mode, roi, counters))

// clang-format off
BOOST_ANNOTATE_MEMBER(acquisition, nb_frames,
    (desc, "number of frames"),
    (doc, "The number of frames to acquire (0 = continuous acquisition)"))

BOOST_ANNOTATE_MEMBER(acquisition, expo_time,
    (desc, "exposure time"),
    (doc, "The exposure time [s]"))

BOOST_ANNOTATE_MEMBER(acquisition, acq_mode,
    (desc, "acquisition mode"),
    (doc, "The acquistion mode [normal, accumulation]"))

BOOST_ANNOTATE_MEMBER(acquisition, roi,
    (desc, "region of interest"),
    (doc, "The region of interest to transfer"))

BOOST_ANNOTATE_MEMBER(acquisition, counters,
    (desc, "region of interest counters"),
    (doc, "A collection of region of interest to compute statistics on"))
// clang-format on

} // namespace lima

template <typename T>
struct is_vector : std::false_type
{
};

template <typename T>
struct is_vector<std::vector<T>> : std::true_type
{
};

template <class T>
inline constexpr bool is_vector_v = is_vector<T>::value;

template <class T, typename Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
void print_with_annotation(std::ostream& os, T const& t, int indent = 0)
{
    bool first = true;
    boost::mp11::mp_for_each<Md>([&](auto D) {
        for (int i = 0; i < indent; i++)
            os << "  ";

        using A = boost::describe::annotate_member<decltype(D)>;

        static_assert(
            std::is_same_v<decltype(D), boost::describe::descriptor_by_pointer<
                                            boost::describe::describe_members<T, boost::describe::mod_any_access>,
                                            decltype(D)::pointer>>);

        os << "\nAnnotations:\n";
        boost::mp11::mp_for_each<A>([&](auto a) {
            os << "." << a.name << " = " << a.value << std::endl;
            ;
        });

        os << "\nDescriptions:\n";
        os << "." << D.name << " = " << t.*D.pointer << std::endl;

        // Recursively print class members if they are described
        using return_t = std::decay_t<boost::callable_traits::return_type_t<decltype(D.pointer)>>;
        if constexpr (std::is_class_v<return_t> && boost::describe::is_described_v<return_t>)
            print_with_annotation(os, t.*D.pointer, indent + 1);
        else if constexpr (is_vector_v<return_t>)
            for (auto&& elem : t.*D.pointer)
                print_with_annotation(os, elem, indent + 1);
    });
}

BOOST_AUTO_TEST_CASE(test_annotation)
{
    using namespace std::chrono_literals;
    using namespace lima;

    using Md = boost::describe::describe_members<acquisition, boost::describe::mod_any_access>;

    using Md0 = boost::describe::descriptor_by_pointer<Md, &acquisition::nb_frames>;
    using Md1 = boost::describe::descriptor_by_pointer<Md, &acquisition::expo_time>;

    // requires C++20
    //using Md2 = boost::describe::descriptor_by_name<Md, "nb_frames">;
    //using Md3 = boost::describe::descriptor_by_name<Md, "expo_time">;

    static_assert(std::is_same_v<Md0, boost::mp11::mp_at_c<Md, 0>>);
    static_assert(std::is_same_v<Md1, boost::mp11::mp_at_c<Md, 1>>);

    static_assert(boost::describe::is_described_v<acquisition>);

    roi_t roi{{0, 0}, {1024, 1024}};
    std::vector<roi_t> counters{{{0, 0}, {256, 256}}, {{512, 512}, {256, 256}}};
    acquisition acq{100, 1s, acq_mode_enum::accumulation, roi, counters};

    print_with_annotation(std::cout, acq);
}

struct base
{
    int foo;
};

struct inherited : base
{
    double bar;
};

BOOST_DESCRIBE_STRUCT(base, (), (foo))
BOOST_DESCRIBE_STRUCT(inherited, (base), (bar))

// clang-format off
BOOST_ANNOTATE_MEMBER(base, foo,
    (desc, "foo"),
    (doc, "The foo integer"))

BOOST_ANNOTATE_MEMBER(inherited, bar,
    (desc, "bar"),
    (doc, "The bar double"))
// clang-format on

BOOST_AUTO_TEST_CASE(test_annotation_normalize)
{
    using Md1 = boost::describe::describe_members<base, boost::describe::mod_any_access>;
    using D1 = boost::mp11::mp_front<Md1>;

    using Md2 = boost::describe::describe_members<inherited, boost::describe::mod_any_access | boost::describe::mod_inherited>;
    using D2 = boost::mp11::mp_front<Md2>;

    using Nd1 = boost::describe::normalize_descriptor<D1>;
    using Nd2 = boost::describe::normalize_descriptor<D2>;

    static_assert(std::is_same_v<Nd1, Nd2>);
}

BOOST_AUTO_TEST_CASE(test_annotation_inherited)
{
    using T = inherited;
    using Md = boost::describe::describe_members<T, boost::describe::mod_any_access | boost::describe::mod_inherited>;

    boost::mp11::mp_for_each<Md>([&](auto D) {
        using A = boost::describe::annotate_member<decltype(D)>;

        std::cout << "\nAnnotations:\n";
        boost::mp11::mp_for_each<A>([&](auto a) {
            std::cout << "." << a.name << " = " << a.value << std::endl;
        });
    });
}
