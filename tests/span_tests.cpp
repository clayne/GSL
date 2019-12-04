///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
// blanket turn off warnings from CppCoreCheck from catch
// so people aren't annoyed by them when running the tool.
#pragma warning(disable : 26440 26426 26497) // from catch

#endif

#if __clang__ || __GNUC__
//disable warnings from gtest
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif

#include <gsl/gsl_byte> // for byte
#include <gsl/gsl_util> // for narrow_cast, at
#include <gsl/span>     // for span, span_iterator, operator==, operator!=
#include <gtest/gtest.h>

#include <array>       // for array
#include <iostream>    // for ptrdiff_t
#include <iterator>    // for reverse_iterator, operator-, operator==
#include <memory>      // for unique_ptr, shared_ptr, make_unique, allo...
#include <regex>       // for match_results, sub_match, match_results<>...
#include <stddef.h>    // for ptrdiff_t
#include <string>      // for string
#include <type_traits> // for integral_constant<>::value, is_default_co...
#include <vector>      // for vector

namespace gsl
{
struct fail_fast;
} // namespace gsl

using namespace std;
using namespace gsl;

namespace
{
struct BaseClass
{
};
struct DerivedClass : BaseClass
{
};
struct AddressOverloaded
{
#if (__cplusplus > 201402L)
    [[maybe_unused]]
#endif
    AddressOverloaded
    operator&() const
    {
        return {};
    }
};
} // namespace

TEST(span_test, constructors)
{
    span<int> s;
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.data(), nullptr);

    span<const int> cs;
    EXPECT_EQ(cs.size(), 0);
    EXPECT_EQ(cs.data(), nullptr);
}

TEST(span_test, constructors_with_extent)
{
    span<int, 0> s;
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.data(), nullptr);

    span<const int, 0> cs;
    EXPECT_EQ(cs.size(), 0);
    EXPECT_EQ(cs.data(), nullptr);
}

TEST(span_test, constructors_with_bracket_init)
{
    span<int> s{};
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.data(), nullptr);

    span<const int> cs{};
    EXPECT_EQ(cs.size(), 0);
    EXPECT_EQ(cs.data(), nullptr);
}

TEST(span_test, size_optimization)
{
    span<int> s;
    EXPECT_EQ(sizeof(s), sizeof(int*) + sizeof(ptrdiff_t));

    span<int, 0> se;
    EXPECT_EQ(sizeof(se), sizeof(int*));
}

TEST(span_test, from_nullptr_size_constructor)
{
    {
        span<int> s{nullptr, narrow_cast<span<int>::index_type>(0)};
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), nullptr);

        span<int> cs{nullptr, narrow_cast<span<int>::index_type>(0)};
        EXPECT_EQ(cs.size(), 0);
        EXPECT_EQ(cs.data(), nullptr);
    }
    {
        auto workaround_macro = []() {
            const span<int, 1> s{nullptr, narrow_cast<span<int>::index_type>(0)};
        };
        EXPECT_DEATH(workaround_macro(), ".*");
    }
    {
        auto workaround_macro = []() { const span<int> s{nullptr, 1}; };
        EXPECT_DEATH(workaround_macro(), ".*");

        auto const_workaround_macro = []() { const span<const int> s{nullptr, 1}; };
        EXPECT_DEATH(const_workaround_macro(), ".*");
    }
    {
        auto workaround_macro = []() { const span<int, 0> s{nullptr, 1}; };
        EXPECT_DEATH(workaround_macro(), ".*");

        auto const_workaround_macro = []() { const span<const int, 0> s{nullptr, 1}; };
        EXPECT_DEATH(const_workaround_macro(), ".*");
    }
    {
        span<int*> s{nullptr, narrow_cast<span<int>::index_type>(0)};
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), nullptr);

        span<const int*> cs{nullptr, narrow_cast<span<int>::index_type>(0)};
        EXPECT_EQ(cs.size(), 0);
        EXPECT_EQ(cs.data(), nullptr);
    }
}

TEST(span_test, from_pointer_length_constructor)
{
    int arr[4] = {1, 2, 3, 4};

    {
        for (int i = 0; i < 4; ++i)
        {
            {
                span<int> s = {&arr[0], i};
                EXPECT_EQ(s.size(), i);
                EXPECT_EQ(s.data(), &arr[0]);
                EXPECT_EQ(s.empty(), i == 0);
                for (int j = 0; j < i; ++j)
                {
                    EXPECT_EQ(arr[j], s[j]);
                    EXPECT_EQ(arr[j], s.at(j));
                    EXPECT_EQ(arr[j], s(j));
                }
            }
            {
                span<int> s = {&arr[i], 4 - narrow_cast<ptrdiff_t>(i)};
                EXPECT_EQ(s.size(), 4 - i);
                EXPECT_EQ(s.data(), &arr[i]);
                EXPECT_EQ(s.empty(), (4 - i) == 0);

                for (int j = 0; j < 4 - i; ++j)
                {
                    EXPECT_EQ(arr[j + i], s[j]);
                    EXPECT_EQ(arr[j + i], s.at(j));
                    EXPECT_EQ(arr[j + i], s(j));
                }
            }
        }
    }

    {
        span<int, 2> s{&arr[0], 2};
        EXPECT_EQ(s.size(), 2);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }

    {
        int* p = nullptr;
        span<int> s{p, narrow_cast<span<int>::index_type>(0)};
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), nullptr);
    }

    {
        int* p = nullptr;
        auto workaround_macro = [=]() { const span<int> s{p, 2}; };
        EXPECT_DEATH(workaround_macro(), ".*");
    }

    {
        auto s = make_span(&arr[0], 2);
        EXPECT_EQ(s.size(), 2);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }

    {
        int* p = nullptr;
        auto s = make_span(p, narrow_cast<span<int>::index_type>(0));
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), nullptr);
    }

    {
        int* p = nullptr;
        auto workaround_macro = [=]() { make_span(p, 2); };
        EXPECT_DEATH(workaround_macro(), ".*");
    }
}

TEST(span_test, from_pointer_pointer_construction)
{
    int arr[4] = {1, 2, 3, 4};

    {
        span<int> s{&arr[0], &arr[2]};
        EXPECT_EQ(s.size(), 2);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }
    {
        span<int, 2> s{&arr[0], &arr[2]};
        EXPECT_EQ(s.size(), 2);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }

    {
        span<int> s{&arr[0], &arr[0]};
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), &arr[0]);
    }

    {
        span<int, 0> s{&arr[0], &arr[0]};
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), &arr[0]);
    }

    // this will fail the std::distance() precondition, which asserts on MSVC debug builds
    //{
    //    auto workaround_macro = [&]() { span<int> s{&arr[1], &arr[0]}; };
    //    EXPECT_DEATH(workaround_macro(), ".*");
    //}

    // this will fail the std::distance() precondition, which asserts on MSVC debug builds
    //{
    //    int* p = nullptr;
    //    auto workaround_macro = [&]() { span<int> s{&arr[0], p}; };
    //    EXPECT_DEATH(workaround_macro(), ".*");
    //}

    {
        int* p = nullptr;
        span<int> s{p, p};
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), nullptr);
    }

    {
        int* p = nullptr;
        span<int, 0> s{p, p};
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), nullptr);
    }

    // this will fail the std::distance() precondition, which asserts on MSVC debug builds
    //{
    //    int* p = nullptr;
    //    auto workaround_macro = [&]() { span<int> s{&arr[0], p}; };
    //    EXPECT_DEATH(workaround_macro(), ".*");
    //}

    {
        auto s = make_span(&arr[0], &arr[2]);
        EXPECT_EQ(s.size(), 2);
        EXPECT_EQ(s.data(), &arr[0]);
        EXPECT_EQ(s[0], 1);
        EXPECT_EQ(s[1], 2);
    }

    {
        auto s = make_span(&arr[0], &arr[0]);
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), &arr[0]);
    }

    {
        int* p = nullptr;
        auto s = make_span(p, p);
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.data(), nullptr);
    }
}

TEST(span_test, from_array_constructor)
 {
     int arr[5] = {1, 2, 3, 4, 5};

     {
         const span<int> s{arr};
         EXPECT_EQ(s.size(), 5);
         EXPECT_EQ(s.data(), &arr[0]);
     }

     {
         const span<int, 5> s{arr};
         EXPECT_EQ(s.size(), 5);
         EXPECT_EQ(s.data(), &arr[0]);
     }

     int arr2d[2][3] = {1, 2, 3, 4, 5, 6};

 #ifdef CONFIRM_COMPILATION_ERRORS
     {
         span<int, 6> s{arr};
     }

     {
         span<int, 0> s{arr};
         EXPECT_EQ(s.size(), 0);
         EXPECT_EQ(s.data(), &arr[0]);
     }

     {
         span<int> s{arr2d};
         EXPECT_EQ(s.size(), 6);
         EXPECT_EQ(s.data(), &arr2d[0][0]);
         EXPECT_EQ(s[0], 1);
         EXPECT_EQ(s[5], 6);
     }

     {
         span<int, 0> s{arr2d};
         EXPECT_EQ(s.size(), 0);
         EXPECT_EQ(s.data(), &arr2d[0][0]);
     }

     {
         span<int, 6> s{arr2d};
     }
 #endif
     {
         const span<int[3]> s{std::addressof(arr2d[0]), 1};
         EXPECT_EQ(s.size(), 1);
         EXPECT_EQ(s.data(), std::addressof(arr2d[0]));
     }

     int arr3d[2][3][2] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

 #ifdef CONFIRM_COMPILATION_ERRORS
     {
         span<int> s{arr3d};
         EXPECT_EQ(s.size(), 12);
         EXPECT_EQ(s.data(), &arr3d[0][0][0]);
         EXPECT_EQ(s[0], 1);
         EXPECT_EQ(s[11], 12);
     }

     {
         span<int, 0> s{arr3d};
         EXPECT_EQ(s.size(), 0);
         EXPECT_EQ(s.data(), &arr3d[0][0][0]);
     }

     {
         span<int, 11> s{arr3d};
     }

     {
         span<int, 12> s{arr3d};
         EXPECT_EQ(s.size(), 12);
         EXPECT_EQ(s.data(), &arr3d[0][0][0]);
         EXPECT_EQ(s[0], 1);
         EXPECT_EQ(s[5], 6);
     }
 #endif
     {
         const span<int[3][2]> s{std::addressof(arr3d[0]), 1};
         EXPECT_EQ(s.size(),  1);
     }

     {
         const auto s = make_span(arr);
         EXPECT_EQ(s.size(),  5);
         EXPECT_EQ(s.data(),std::addressof(arr[0]));
     }

     {
         const auto s = make_span(std::addressof(arr2d[0]), 1);
         EXPECT_EQ(s.size(),  1);
         EXPECT_EQ(s.data(), std::addressof(arr2d[0]));
     }

     {
         const auto s = make_span(std::addressof(arr3d[0]), 1);
         EXPECT_EQ(s.size(),  1);
         EXPECT_EQ(s.data(), std::addressof(arr3d[0]));
     }

     AddressOverloaded ao_arr[5] = {};

     {
         const span<AddressOverloaded, 5> s{ao_arr};
         EXPECT_EQ(s.size(),  5);
         EXPECT_EQ(s.data(), std::addressof(ao_arr[0]));
     }
 }

 TEST(span_test, from_dynamic_array_constructor)
 {
     double(*arr)[3][4] = new double[100][3][4];

     {
         span<double> s(&arr[0][0][0], 10);
         EXPECT_EQ(s.size(),  10); EXPECT_EQ(s.data(), &arr[0][0][0]);
     }

     {
         auto s = make_span(&arr[0][0][0], 10);
         EXPECT_EQ(s.size(),  10); EXPECT_EQ(s.data(), &arr[0][0][0]);
     }

     delete[] arr;
 }

 TEST(span_test, from_std_array_constructor)
 {
     std::array<int, 4> arr = {1, 2, 3, 4};

     {
         span<int> s{arr};
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());

         span<const int> cs{arr};
         EXPECT_EQ(cs.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(cs.data(), arr.data());
     }

     {
         span<int, 4> s{arr};
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());

         span<const int, 4> cs{arr};
         EXPECT_EQ(cs.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(cs.data(), arr.data());
     }

     {
         std::array<int, 0> empty_arr{};
         span<int> s{empty_arr};
         EXPECT_EQ(s.size(), 0);
         EXPECT_TRUE(s.empty());
     }

     std::array<AddressOverloaded, 4> ao_arr{};

     {
         span<AddressOverloaded, 4> fs{ao_arr};
         EXPECT_EQ(fs.size(), narrow_cast<ptrdiff_t>(ao_arr.size()));
         EXPECT_EQ(ao_arr.data(), fs.data());
     }

 #ifdef CONFIRM_COMPILATION_ERRORS
     {
         span<int, 2> s{arr};
         EXPECT_EQ(s.size(),  2); EXPECT_EQ(s.data(), arr.data());

         span<const int, 2> cs{arr};
         EXPECT_EQ(cs.size(),  2); EXPECT_EQ(cs.data(), arr.data());
     }

     {
         span<int, 0> s{arr};
         EXPECT_EQ(s.size(),  0); EXPECT_EQ(s.data(), arr.data());

         span<const int, 0> cs{arr};
         EXPECT_EQ(cs.size(),  0); EXPECT_EQ(cs.data(), arr.data());
     }

     {
         span<int, 5> s{arr};
     }

     {
         auto get_an_array = []() -> std::array<int, 4> { return {1, 2, 3, 4}; };
         auto take_a_span = [](span<int> s) { static_cast<void>(s); };
         // try to take a temporary std::array
         take_a_span(get_an_array());
     }
 #endif

     {
         auto get_an_array = []() -> std::array<int, 4> { return {1, 2, 3, 4}; };
         auto take_a_span = [](span<const int> s) { static_cast<void>(s); };
         // try to take a temporary std::array
         take_a_span(get_an_array());
     }

     {
         auto s = make_span(arr);
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());
     }

     // This test checks for the bug found in gcc 6.1, 6.2, 6.3, 6.4, 6.5 7.1, 7.2, 7.3 - issue #590
     {
         span<int> s1 = make_span(arr);

         static span<int> s2;
         s2 = s1;

 #if defined(__GNUC__) && __GNUC__ == 6 && (__GNUC_MINOR__ == 4 || __GNUC_MINOR__ == 5) &&          \
     __GNUC_PATCHLEVEL__ == 0 && defined(__OPTIMIZE__)
         // Known to be broken in gcc 6.4 and 6.5 with optimizations
         // Issue in gcc: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83116
         EXPECT_EQ(s1.size(), 4);
         EXPECT_EQ(s2.size(), 0);
 #else
         EXPECT_EQ(s1.size(), s2.size());
 #endif
     }
 }

 TEST(span_test, from_const_std_array_constructor)
 {
     const std::array<int, 4> arr = {1, 2, 3, 4};

     {
         span<const int> s{arr};
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());
     }

     {
         span<const int, 4> s{arr};
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());
     }

     const std::array<AddressOverloaded, 4> ao_arr{};

     {
         span<const AddressOverloaded, 4> s{ao_arr};
         EXPECT_EQ(s.size(), narrow_cast<ptrdiff_t>(ao_arr.size()));
         EXPECT_EQ(s.data(), ao_arr.data());
     }

 #ifdef CONFIRM_COMPILATION_ERRORS
     {
         span<const int, 2> s{arr};
         EXPECT_EQ(s.size(),  2); EXPECT_EQ(s.data(), arr.data());
     }

     {
         span<const int, 0> s{arr};
         EXPECT_EQ(s.size(),  0); EXPECT_EQ(s.data(), arr.data());
     }

     {
         span<const int, 5> s{arr};
     }
 #endif

     {
         auto get_an_array = []() -> const std::array<int, 4> { return {1, 2, 3, 4}; };
         auto take_a_span = [](span<const int> s) { static_cast<void>(s); };
         // try to take a temporary std::array
         take_a_span(get_an_array());
     }

     {
         auto s = make_span(arr);
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());
     }
 }

 TEST(span_test, from_std_array_const_constructor)
 {
     std::array<const int, 4> arr = {1, 2, 3, 4};

     {
         span<const int> s{arr};
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());
     }

     {
         span<const int, 4> s{arr};
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());
     }

 #ifdef CONFIRM_COMPILATION_ERRORS
     {
         span<const int, 2> s{arr};
         EXPECT_EQ(s.size(),  2); EXPECT_EQ(s.data(), arr.data());
     }

     {
         span<const int, 0> s{arr};
         EXPECT_EQ(s.size(),  0); EXPECT_EQ(s.data(), arr.data());
     }

     {
         span<const int, 5> s{arr};
     }

     {
         span<int, 4> s{arr};
     }
 #endif

     {
         auto s = make_span(arr);
         EXPECT_EQ(s.size(),  narrow_cast<ptrdiff_t>(arr.size())); EXPECT_EQ(s.data(), arr.data());
     }
 }

 TEST(span_test, from_container_constructor)
 {
     std::vector<int> v = {1, 2, 3};
     const std::vector<int> cv = v;

     {
         span<int> s{v};
         EXPECT_EQ(s.size(),  narrow_cast<std::ptrdiff_t>(v.size())); EXPECT_EQ(s.data(), v.data());

         span<const int> cs{v};
         EXPECT_EQ(cs.size(),  narrow_cast<std::ptrdiff_t>(v.size())); EXPECT_EQ(cs.data(), v.data());
     }

     std::string str = "hello";
     const std::string cstr = "hello";

     {
 #ifdef CONFIRM_COMPILATION_ERRORS
         span<char> s{str};
         EXPECT_EQ(s.size(), narrow_cast<std::ptrdiff_t>(str.size()));
         EXPECT_EQ(s.data(), str.data()));
 #endif
         span<const char> cs{str};
         EXPECT_EQ(cs.size(), narrow_cast<std::ptrdiff_t>(str.size()));
         EXPECT_EQ(cs.data(), str.data());
     }

     {
 #ifdef CONFIRM_COMPILATION_ERRORS
         span<char> s{cstr};
 #endif
         span<const char> cs{cstr};
         EXPECT_EQ(cs.size(), narrow_cast<std::ptrdiff_t>(cstr.size()));
         EXPECT_EQ(cs.data(), cstr.data());
     }

     {
 #ifdef CONFIRM_COMPILATION_ERRORS
         auto get_temp_vector = []() -> std::vector<int> { return {}; };
         auto use_span = [](span<int> s) { static_cast<void>(s); };
         use_span(get_temp_vector());
 #endif
     }

     {
         auto get_temp_vector = []() -> std::vector<int> { return {}; };
         auto use_span = [](span<const int> s) { static_cast<void>(s); };
         use_span(get_temp_vector());
     }

     {
 #ifdef CONFIRM_COMPILATION_ERRORS
         auto get_temp_string = []() -> std::string { return {}; };
         auto use_span = [](span<char> s) { static_cast<void>(s); };
         use_span(get_temp_string());
 #endif
     }

     {
         auto get_temp_string = []() -> std::string { return {}; };
         auto use_span = [](span<const char> s) { static_cast<void>(s); };
         use_span(get_temp_string());
     }

     {
 #ifdef CONFIRM_COMPILATION_ERRORS
         auto get_temp_vector = []() -> const std::vector<int> { return {}; };
         auto use_span = [](span<const char> s) { static_cast<void>(s); };
         use_span(get_temp_vector());
 #endif
     }

     {
         auto get_temp_string = []() -> const std::string { return {}; };
         auto use_span = [](span<const char> s) { static_cast<void>(s); };
         use_span(get_temp_string());
     }

     {
 #ifdef CONFIRM_COMPILATION_ERRORS
         std::map<int, int> m;
         span<int> s{m};
 #endif
     }

     {
         auto s = make_span(v);
         EXPECT_EQ(s.size(),  narrow_cast<std::ptrdiff_t>(v.size())); EXPECT_EQ(s.data(), v.data());

         auto cs = make_span(cv);
         EXPECT_EQ(cs.size(), narrow_cast<std::ptrdiff_t>(cv.size()));
         EXPECT_EQ(cs.data(), cv.data());
     }
 }

 TEST(span_test, from_convertible_span_constructor){{span<DerivedClass> avd;
 span<const DerivedClass> avcd = avd;
 static_cast<void>(avcd);
 }

 {
 #ifdef CONFIRM_COMPILATION_ERRORS
     span<DerivedClass> avd;
     span<BaseClass> avb = avd;
     static_cast<void>(avb);
 #endif
 }

 #ifdef CONFIRM_COMPILATION_ERRORS
 {
     span<int> s;
     span<unsigned int> s2 = s;
     static_cast<void>(s2);
 }

 {
     span<int> s;
     span<const unsigned int> s2 = s;
     static_cast<void>(s2);
 }

 {
     span<int> s;
     span<short> s2 = s;
     static_cast<void>(s2);
 }
 #endif
 }

 TEST(span_test, copy_move_and_assignment)
 {
     span<int> s1;
     EXPECT_TRUE(s1.empty());

     int arr[] = {3, 4, 5};

     span<const int> s2 = arr;
     EXPECT_EQ(s2.size(),  3); EXPECT_EQ(s2.data(), &arr[0]);

     s2 = s1;
     EXPECT_TRUE(s2.empty());

     auto get_temp_span = [&]() -> span<int> { return {&arr[1], 2}; };
     auto use_span = [&](span<const int> s) { EXPECT_EQ(s.size(),  2); EXPECT_EQ(s.data(), &arr[1]);
     }; use_span(get_temp_span());

     s1 = get_temp_span();
     EXPECT_EQ(s1.size(),  2); EXPECT_EQ(s1.data(), &arr[1]);
 }

 TEST(span_test, first)
 {
     int arr[5] = {1, 2, 3, 4, 5};

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.first<2>().size(), 2);
         EXPECT_EQ(av.first(2).size(), 2);
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.first<0>().size(), 0);
         EXPECT_EQ(av.first(0).size(), 0);
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.first<5>().size(), 5);
         EXPECT_EQ(av.first(5).size(), 5);
     }

     {
         span<int, 5> av = arr;
 #ifdef CONFIRM_COMPILATION_ERRORS
         EXPECT_EQ(av.first<6>().size(), 6);
         EXPECT_EQ(av.first<-1>().size(), -1);
 #endif
         EXPECT_DEATH(av.first(6).size(), ".*");
     }

     {
         span<int> av;
         EXPECT_EQ(av.first<0>().size(), 0);
         EXPECT_EQ(av.first(0).size(), 0);
     }
 }

 TEST(span_test, last)
 {
     int arr[5] = {1, 2, 3, 4, 5};

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.last<2>().size(), 2);
         EXPECT_EQ(av.last(2).size(), 2);
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.last<0>().size(), 0);
         EXPECT_EQ(av.last(0).size(), 0);
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.last<5>().size(), 5);
         EXPECT_EQ(av.last(5).size(), 5);
     }

     {
         span<int, 5> av = arr;
 #ifdef CONFIRM_COMPILATION_ERRORS
         EXPECT_EQ(av.last<6>().size(), 6);
 #endif
         EXPECT_DEATH(av.last(6).size(), ".*");
     }

     {
         span<int> av;
         EXPECT_EQ(av.last<0>().size(), 0);
         EXPECT_EQ(av.last(0).size(), 0);
     }
 }

 TEST(span_test, subspan)
 {
     int arr[5] = {1, 2, 3, 4, 5};

     {
         span<int, 5> av = arr;
         EXPECT_EQ((av.subspan<2, 2>().size()), 2);
         EXPECT_EQ(decltype(av.subspan<2, 2>())::extent, 2);
         EXPECT_EQ(av.subspan(2, 2).size(), 2);
         EXPECT_EQ(av.subspan(2, 3).size(), 3);
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ((av.subspan<0, 0>().size()), 0);
         EXPECT_EQ(decltype(av.subspan<0, 0>())::extent, 0);
         EXPECT_EQ(av.subspan(0, 0).size(), 0);
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ((av.subspan<0, 5>().size()), 5);
         EXPECT_EQ(decltype(av.subspan<0, 5>())::extent, 5);
         EXPECT_EQ(av.subspan(0, 5).size(), 5);

         EXPECT_DEATH(av.subspan(0, 6).size(), ".*");
         EXPECT_DEATH(av.subspan(1, 5).size(), ".*");
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ((av.subspan<4, 0>().size()), 0);
         EXPECT_EQ(decltype(av.subspan<4, 0>())::extent, 0);
         EXPECT_EQ(av.subspan(4, 0).size(), 0);
         EXPECT_EQ(av.subspan(5, 0).size(), 0);
         EXPECT_DEATH(av.subspan(6, 0).size(), ".*");
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.subspan<1>().size(), 4);
         EXPECT_EQ(decltype(av.subspan<1>())::extent, 4);
     }

     {
         span<int> av;
         EXPECT_EQ((av.subspan<0, 0>().size()), 0);
         EXPECT_EQ(decltype(av.subspan<0, 0>())::extent, 0);
         EXPECT_EQ(av.subspan(0, 0).size(), 0);
         EXPECT_DEATH((av.subspan<1, 0>().size()), ".*");
     }

     {
         span<int> av;
         EXPECT_EQ(av.subspan(0).size(), 0);
         EXPECT_DEATH(av.subspan(1).size(), ".*");
     }

     {
         span<int> av = arr;
         EXPECT_EQ(av.subspan(0).size(), 5);
         EXPECT_EQ(av.subspan(1).size(), 4);
         EXPECT_EQ(av.subspan(4).size(), 1);
         EXPECT_EQ(av.subspan(5).size(), 0);
         EXPECT_DEATH(av.subspan(6).size(), ".*");
         const auto av2 = av.subspan(1);
         for (int i = 0; i < 4; ++i) EXPECT_EQ(av2[i], i + 2);
     }

     {
         span<int, 5> av = arr;
         EXPECT_EQ(av.subspan(0).size(), 5);
         EXPECT_EQ(av.subspan(1).size(), 4);
         EXPECT_EQ(av.subspan(4).size(), 1);
         EXPECT_EQ(av.subspan(5).size(), 0);
         EXPECT_DEATH(av.subspan(6).size(), ".*");
         const auto av2 = av.subspan(1);
         for (int i = 0; i < 4; ++i) EXPECT_EQ(av2[i], i + 2);
     }
 }

 TEST(span_test, at_call)
 {
     int arr[4] = {1, 2, 3, 4};

     {
         span<int> s = arr;
         EXPECT_EQ(s.at(0), 1);
         EXPECT_DEATH(s.at(5), ".*");
     }

     {
         int arr2d[2] = {1, 6};
         span<int, 2> s = arr2d;
         EXPECT_EQ(s.at(0), 1);
         EXPECT_EQ(s.at(1), 6);
         EXPECT_DEATH(s.at(2), ".*");
     }
 }

 TEST(span_test, operator_function_call)
 {
     int arr[4] = {1, 2, 3, 4};

     {
         span<int> s = arr;
         EXPECT_EQ(s(0), 1);
         EXPECT_DEATH(s(5), ".*");
     }

     {
         int arr2d[2] = {1, 6};
         span<int, 2> s = arr2d;
         EXPECT_EQ(s(0), 1);
         EXPECT_EQ(s(1), 6);
         EXPECT_DEATH(s(2), ".*");
     }
 }

 TEST(span_test, iterator_default_init)
 {
     span<int>::iterator it1;
     span<int>::iterator it2;
     EXPECT_EQ(it1, it2);
 }

 TEST(span_test, const_iterator_default_init)
 {
     span<int>::const_iterator it1;
     span<int>::const_iterator it2;
     EXPECT_EQ(it1, it2);
 }

 TEST(span_test, iterator_conversions)
 {
     span<int>::iterator badIt;
     span<int>::const_iterator badConstIt;
     EXPECT_EQ(badIt, badConstIt);

     int a[] = {1, 2, 3, 4};
     span<int> s = a;

     auto it = s.begin();
     auto cit = s.cbegin();

     EXPECT_EQ(it, cit);
     EXPECT_EQ(cit, it);

     span<int>::const_iterator cit2 = it;
     EXPECT_EQ(cit2, cit);

     span<int>::const_iterator cit3 = it + 4;
     EXPECT_EQ(cit3, s.cend());
 }

 TEST(span_test, iterator_comparisons)
 {
     int a[] = {1, 2, 3, 4};
     {
         span<int> s = a;
         span<int>::iterator it = s.begin();
         auto it2 = it + 1;
         span<int>::const_iterator cit = s.cbegin();

         EXPECT_EQ(it, cit);
         EXPECT_EQ(cit, it);
         EXPECT_EQ(it, it);
         EXPECT_EQ(cit, cit);
         EXPECT_EQ(cit, s.begin());
         EXPECT_EQ(s.begin(), cit);
         EXPECT_EQ(s.cbegin(), cit);
         EXPECT_EQ(it, s.begin());
         EXPECT_EQ(s.begin(), it);

         EXPECT_NE(it, it2);
         EXPECT_NE(it2, it);
         EXPECT_NE(it, s.end());
         EXPECT_NE(it2, s.end());
         EXPECT_NE(s.end(), it);
         EXPECT_NE(it2, cit);
         EXPECT_NE(cit, it2);

         EXPECT_LT(it, it2);
         EXPECT_LE(it, it2);
         EXPECT_LE(it2, s.end());
         EXPECT_LT(it, s.end());
         EXPECT_LE(it, cit);
         EXPECT_LE(cit, it);
         EXPECT_LT(cit, it2);
         EXPECT_LE(cit, it2);
         EXPECT_LT(cit, s.end());
         EXPECT_LE(cit, s.end());

         EXPECT_GT(it2, it);
         EXPECT_GE(it2, it);
         EXPECT_GT(s.end(), it2);
         EXPECT_GE(s.end(), it2);
         EXPECT_GT(it2, cit);
         EXPECT_GE(it2, cit);
     }
 }

 TEST(span_test, begin_end)
 {
     {
         int a[] = {1, 2, 3, 4};
         span<int> s = a;

         span<int>::iterator it = s.begin();
         span<int>::iterator it2 = std::begin(s);
         EXPECT_EQ(it, it2);

         it = s.end();
         it2 = std::end(s);
         EXPECT_EQ(it, it2);
     }

     {
         int a[] = {1, 2, 3, 4};
         span<int> s = a;

         auto it = s.begin();
         auto first = it;
         EXPECT_EQ(it, first);
         EXPECT_EQ(*it, 1);

         auto beyond = s.end();
         EXPECT_NE(it, beyond);
         EXPECT_DEATH(*beyond, ".*");

         EXPECT_EQ(beyond - first, 4);
         EXPECT_EQ(first - first, 0);
         EXPECT_EQ(beyond - beyond, 0);

         ++it;
         EXPECT_EQ(it - first, 1);
         EXPECT_EQ(*it, 2);
         *it = 22;
         EXPECT_EQ(*it, 22);
         EXPECT_EQ(beyond - it, 3);

         it = first;
         EXPECT_EQ(it, first);
         while (it != s.end())
         {
             *it = 5;
             ++it;
         }

         EXPECT_EQ(it, beyond);
         EXPECT_EQ(it - beyond, 0);

         for (const auto& n : s) { EXPECT_EQ(n, 5); }
     }
 }

 TEST(span_test, cbegin_cend)
 {
     {
         int a[] = {1, 2, 3, 4};
         span<int> s = a;

         span<int>::const_iterator cit = s.cbegin();
         span<int>::const_iterator cit2 = std::cbegin(s);
         EXPECT_EQ(cit, cit2);

         cit = s.cend();
         cit2 = std::cend(s);
         EXPECT_EQ(cit, cit2);
     }

     {
         int a[] = {1, 2, 3, 4};
         span<int> s = a;

         auto it = s.cbegin();
         auto first = it;
         EXPECT_EQ(it, first);
         EXPECT_EQ(*it, 1);

         auto beyond = s.cend();
         EXPECT_NE(it, beyond);
         EXPECT_DEATH(*beyond, ".*");

         EXPECT_EQ(beyond - first, 4);
         EXPECT_EQ(first - first, 0);
         EXPECT_EQ(beyond - beyond, 0);

         ++it;
         EXPECT_EQ(it - first, 1);
         EXPECT_EQ(*it, 2);
         EXPECT_EQ(beyond - it, 3);

         int last = 0;
         it = first;
         EXPECT_EQ(it, first);
         while (it != s.cend())
         {
             EXPECT_EQ(*it, last + 1);

             last = *it;
             ++it;
         }

         EXPECT_EQ(it, beyond);
         EXPECT_EQ(it - beyond, 0);
     }
 }

 TEST(span_test, rbegin_rend)
 {
     {
         int a[] = {1, 2, 3, 4};
         span<int> s = a;

         auto it = s.rbegin();
         auto first = it;
         EXPECT_EQ(it, first);
         EXPECT_EQ(*it, 4);

         auto beyond = s.rend();
         EXPECT_NE(it, beyond);
         //EXPECT_DEATH(*beyond, ".*");

         EXPECT_EQ(beyond - first, 4);
         EXPECT_EQ(first - first, 0);
         EXPECT_EQ(beyond - beyond, 0);

         ++it;
         EXPECT_EQ(it - first, 1);
         EXPECT_EQ(*it, 3);
         *it = 22;
         EXPECT_EQ(*it, 22);
         EXPECT_EQ(beyond - it, 3);

         it = first;
         EXPECT_EQ(it, first);
         while (it != s.rend())
         {
             *it = 5;
             ++it;
         }

         EXPECT_EQ(it, beyond);
         EXPECT_EQ(it - beyond, 0);

         for (const auto& n : s) { EXPECT_EQ(n, 5); }
     }
 }

 TEST(span_test, crbegin_crend)
 {
     {
         int a[] = {1, 2, 3, 4};
         span<int> s = a;

         auto it = s.crbegin();
         auto first = it;
         EXPECT_EQ(it, first);
         EXPECT_EQ(*it, 4);

         auto beyond = s.crend();
         EXPECT_NE(it, beyond);
         //EXPECT_DEATH(*beyond, ".*");

         EXPECT_EQ(beyond - first, 4);
         EXPECT_EQ(first - first, 0);
         EXPECT_EQ(beyond - beyond, 0);

         ++it;
         EXPECT_EQ(it - first, 1);
         EXPECT_EQ(*it, 3);
         EXPECT_EQ(beyond - it, 3);

         it = first;
         EXPECT_EQ(it, first);
         int last = 5;
         while (it != s.crend())
         {
             EXPECT_EQ(*it, last - 1);
             last = *it;

             ++it;
         }

         EXPECT_EQ(it, beyond);
         EXPECT_EQ(it - beyond, 0);
     }
 }

 TEST(span_test, comparison_operators)
 {
     {
         span<int> s1;
         span<int> s2;
         EXPECT_EQ(s1, s2);
         EXPECT_FALSE(s1 != s2);
         EXPECT_FALSE(s1 < s2);
         EXPECT_LE(s1, s2);
         EXPECT_FALSE(s1 > s2);
         EXPECT_GE(s1, s2);
         EXPECT_EQ(s2, s1);
         EXPECT_FALSE(s2 != s1);
         EXPECT_FALSE(s2 != s1);
         EXPECT_LE(s2, s1);
         EXPECT_FALSE(s2 > s1);
         EXPECT_GE(s2, s1);
     }

     {
         int arr[] = {2, 1};
         span<int> s1 = arr;
         span<int> s2 = arr;

         EXPECT_EQ(s1, s2);
         EXPECT_FALSE(s1 != s2);
         EXPECT_FALSE(s1 < s2);
         EXPECT_LE(s1, s2);
         EXPECT_FALSE(s1 > s2);
         EXPECT_GE(s1, s2);
         EXPECT_EQ(s2, s1);
         EXPECT_FALSE(s2 != s1);
         EXPECT_FALSE(s2 < s1);
         EXPECT_LE(s2, s1);
         EXPECT_FALSE(s2 > s1);
         EXPECT_GE(s2, s1);
     }

     {
         int arr[] = {2, 1}; // bigger

         span<int> s1;
         span<int> s2 = arr;

         EXPECT_NE(s1, s2);
         EXPECT_NE(s2, s1);
         EXPECT_FALSE(s1 == s2);
         EXPECT_FALSE(s2 == s1);
         EXPECT_LT(s1, s2);
         EXPECT_FALSE(s2 < s1);
         EXPECT_LE(s1, s2);
         EXPECT_FALSE(s2 <= s1);
         EXPECT_GT(s2, s1);
         EXPECT_FALSE(s1 > s2);
         EXPECT_GE(s2, s1);
         EXPECT_FALSE(s1 >= s2);
     }

     {
         int arr1[] = {1, 2};
         int arr2[] = {1, 2};
         span<int> s1 = arr1;
         span<int> s2 = arr2;

         EXPECT_EQ(s1, s2);
         EXPECT_FALSE(s1 != s2);
         EXPECT_FALSE(s1 < s2);
         EXPECT_LE(s1, s2);
         EXPECT_FALSE(s1 > s2);
         EXPECT_GE(s1, s2);
         EXPECT_EQ(s2, s1);
         EXPECT_FALSE(s2 != s1);
         EXPECT_FALSE(s2 < s1);
         EXPECT_LE(s2, s1);
         EXPECT_FALSE(s2 > s1);
         EXPECT_GE(s2, s1);
     }

     {
         int arr[] = {1, 2, 3};

         span<int> s1 = {&arr[0], 2}; // shorter
         span<int> s2 = arr;          // longer

         EXPECT_NE(s1, s2);
         EXPECT_NE(s2, s1);
         EXPECT_FALSE(s1 == s2);
         EXPECT_FALSE(s2 == s1);
         EXPECT_LT(s1, s2);
         EXPECT_FALSE(s2 < s1);
         EXPECT_LE(s1, s2);
         EXPECT_FALSE(s2 <= s1);
         EXPECT_GT(s2, s1);
         EXPECT_FALSE(s1 > s2);
         EXPECT_GE(s2, s1);
         EXPECT_FALSE(s1 >= s2);
     }

     {
         int arr1[] = {1, 2}; // smaller
         int arr2[] = {2, 1}; // bigger

         span<int> s1 = arr1;
         span<int> s2 = arr2;

         EXPECT_NE(s1, s2);
         EXPECT_NE(s2, s1);
         EXPECT_FALSE(s1 == s2);
         EXPECT_FALSE(s2 == s1);
         EXPECT_LT(s1, s2);
         EXPECT_FALSE(s2 < s1);
         EXPECT_LE(s1, s2);
         EXPECT_FALSE(s2 <= s1);
         EXPECT_GT(s2, s1);
         EXPECT_FALSE(s1 > s2);
         EXPECT_GE(s2, s1);
         EXPECT_FALSE(s1 >= s2);
     }
 }

 TEST(span_test, as_bytes)
 {
     int a[] = {1, 2, 3, 4};

     {
         const span<const int> s = a;
         EXPECT_EQ(s.size(), 4);
         const span<const byte> bs = as_bytes(s);
         EXPECT_EQ(static_cast<const void*>(bs.data()), static_cast<const void*>(s.data()));
         EXPECT_EQ(bs.size(), s.size_bytes());
     }

     {
         span<int> s;
         const auto bs = as_bytes(s);
         EXPECT_EQ(bs.size(), s.size());
         EXPECT_EQ(bs.size(), 0);
         EXPECT_EQ(bs.size_bytes(), 0);
         EXPECT_EQ(static_cast<const void*>(bs.data()), static_cast<const void*>(s.data()));
         EXPECT_EQ(bs.data(), nullptr);
     }

     {
         span<int> s = a;
         const auto bs = as_bytes(s);
         EXPECT_EQ(static_cast<const void*>(bs.data()), static_cast<const void*>(s.data()));
         EXPECT_EQ(bs.size(), s.size_bytes());
     }
 }

 TEST(span_test, as_writeable_bytes)
 {
     int a[] = {1, 2, 3, 4};

     {
 #ifdef CONFIRM_COMPILATION_ERRORS
         // you should not be able to get writeable bytes for const objects
         span<const int> s = a;
         EXPECT_EQ(s.size(), 4);
         span<const byte> bs = as_writeable_bytes(s);
         EXPECT_EQ(static_cast<void*>(bs.data()), static_cast<void*>(s.data()));
         EXPECT_EQ(bs.size(), s.size_bytes());
 #endif
     }

     {
         span<int> s;
         const auto bs = as_writeable_bytes(s);
         EXPECT_EQ(bs.size(), s.size());
         EXPECT_EQ(bs.size(), 0);
         EXPECT_EQ(bs.size_bytes(), 0);
         EXPECT_EQ(static_cast<void*>(bs.data()), static_cast<void*>(s.data()));
         EXPECT_EQ(bs.data(), nullptr);
     }

     {
         span<int> s = a;
         const auto bs = as_writeable_bytes(s);
         EXPECT_EQ(static_cast<void*>(bs.data()), static_cast<void*>(s.data()));
         EXPECT_EQ(bs.size(), s.size_bytes());
     }
 }

 TEST(span_test, fixed_size_conversions)
 {
     int arr[] = {1, 2, 3, 4};

     // converting to an span from an equal size array is ok
     span<int, 4> s4 = arr;
     EXPECT_EQ(s4.size(), 4);

     // converting to dynamic_range is always ok
     {
         span<int> s = s4;
         EXPECT_EQ(s.size(), s4.size());
         static_cast<void>(s);
     }

 // initialization or assignment to static span that REDUCES size is NOT ok
 #ifdef CONFIRM_COMPILATION_ERRORS
     {
         span<int, 2> s = arr;
     }
     {
         span<int, 2> s2 = s4;
         static_cast<void>(s2);
     }
 #endif

     // even when done dynamically
     {
         span<int> s = arr;
         auto f = [&]() {
             const span<int, 2> s2 = s;
             static_cast<void>(s2);
         };
         EXPECT_DEATH(f(), ".*");
     }

     // but doing so explicitly is ok

     // you can convert statically
     {
         const span<int, 2> s2 = {&arr[0], 2};
         static_cast<void>(s2);
     }
     {
         const span<int, 1> s1 = s4.first<1>();
         static_cast<void>(s1);
     }

     // ...or dynamically
     {
         // NB: implicit conversion to span<int,1> from span<int>
         span<int, 1> s1 = s4.first(1);
         static_cast<void>(s1);
     }

     // initialization or assignment to static span that requires size INCREASE is not ok.
     int arr2[2] = {1, 2};

 #ifdef CONFIRM_COMPILATION_ERRORS
     {
         span<int, 4> s3 = arr2;
     }
     {
         span<int, 2> s2 = arr2;
         span<int, 4> s4a = s2;
     }
 #endif
     {
         auto f = [&]() {
             const span<int, 4> _s4 = {arr2, 2};
             static_cast<void>(_s4);
         };
         EXPECT_DEATH(f(), ".*");
     }

     // this should fail - we are trying to assign a small dynamic span to a fixed_size larger one
     span<int> av = arr2; auto f = [&]() {
         const span<int, 4> _s4 = av;
         static_cast<void>(_s4);
     };
     EXPECT_DEATH(f(), ".*");
 }

 TEST(span_test, interop_with_std_regex)
 {
     char lat[] = {'1', '2', '3', '4', '5', '6', 'E', 'F', 'G'};
     span<char> s = lat;
     const auto f_it = s.begin() + 7;

     std::match_results<span<char>::iterator> match;

     std::regex_match(s.begin(), s.end(), match, std::regex(".*"));
     EXPECT_TRUE(match.ready());
     EXPECT_FALSE(match.empty());
     EXPECT_TRUE(match[0].matched);
     EXPECT_EQ(match[0].first, s.begin());
     EXPECT_EQ(match[0].second, s.end());

     std::regex_search(s.begin(), s.end(), match, std::regex("F"));
     EXPECT_TRUE(match.ready());
     EXPECT_FALSE(match.empty());
     EXPECT_TRUE(match[0].matched);
     EXPECT_EQ(match[0].first, f_it);
     EXPECT_EQ(match[0].second, (f_it + 1));
 }

 TEST(span_test, interop_with_gsl_at)
 {
     int arr[5] = {1, 2, 3, 4, 5};
     span<int> s{arr};
     EXPECT_EQ(at(s, 0),  1); EXPECT_EQ(at(s, 1), 2);
 }

 TEST(span_test, default_constructible)
 {
     EXPECT_TRUE((std::is_default_constructible<span<int>>::value));
     EXPECT_TRUE((std::is_default_constructible<span<int, 0>>::value));
     EXPECT_FALSE((std::is_default_constructible<span<int, 42>>::value));
 }

#if __clang__ || __GNUC__
#pragma GCC diagnostic pop
#endif
