// vim: sw=3 ts=3 expandtab cindent
#include "bits/exceptions.h"
#include <gtest/gtest.h>

namespace {

using namespace epolling;
using std::make_error_code;
using std::rand;
using std::experimental::string_view;
using std::error_code;
using std::system_error;


TEST(safe, using_exceptional_overload_when_function_returns_negative_should_throw_system_error_with_errno_as_code_and_what_should_contain_passed_in_error_description) {
   // Given
   error_code expected_code = make_error_code(static_cast<std::errc>(rand() % 5 + 1));
   string_view passed_in_what = "Some string";
   error_code actual_code;
   bool contains_passed_in_what = false;
   auto f = [expected_code] {
      errno = expected_code.value();
      return -1;
   };

   // When
   try {
      safe(f, passed_in_what);
   }
   catch (const system_error &e) {
      actual_code = e.code();
      contains_passed_in_what = string_view{e.what()}.find(passed_in_what) != string_view::npos;
   }

   // Then
   ASSERT_EQ(expected_code, actual_code);
   ASSERT_TRUE(contains_passed_in_what);
}


TEST(safe, using_exceptional_overload_when_function_returns_nonnegative_should_return_function_return_value) {
   // Given
   int expected = rand() % 500;
   auto f = [expected] {
      return expected;
   };

   // When
   int actual = safe(f, "");

   // Then
   ASSERT_EQ(expected, actual);
}


TEST(safe, using_unexceptional_overload_when_function_returns_nonnegative_should_return_function_return_value) {
   // Given
   std::error_code ec;
   int expected = rand() % 500;
   auto f = [expected] {
      return expected;
   };

   // When
   int actual = safe(f, ec);

   // Then
   ASSERT_EQ(expected, actual);
}


TEST(safe, using_unexceptional_overload_when_function_returns_negative_should_return_function_return_value_and_set_code_to_errno) {
   // Given
   error_code expected_code = make_error_code(static_cast<std::errc>(rand() % 5 + 1));
   int expected = -(rand() % 500);
   auto f = [expected, expected_code] {
      errno = expected_code.value();
      return expected;
   };
   std::error_code actual_code;

   // When
   int actual = safe(f, actual_code);

   // Then
   ASSERT_EQ(expected, actual);
   ASSERT_EQ(expected_code, actual_code);
}


TEST(safe, using_unexceptional_overload_when_function_throws_system_error_then_should_return_negative_1_and_set_code_to_system_error_code) {
   // Given
   error_code expected_code = make_error_code(static_cast<std::errc>(rand() % 5 + 1));
   int expected = -1;
   auto f = [expected_code] () -> int {
      throw system_error(expected_code);
   };
   std::error_code actual_code;

   // When
   int actual = safe(f, actual_code);

   // Then
   ASSERT_EQ(expected, actual);
   ASSERT_EQ(expected_code, actual_code);
}


TEST(safe, using_unexceptional_overload_when_function_throws_non_system_error_then_should_return_negative_1_and_set_code_to_errno) {
   // Given
   error_code expected_code = make_error_code(static_cast<std::errc>(rand() % 5 + 1));
   int expected = -1;
   auto f = [expected_code] () -> int {
      errno = expected_code.value();
      throw "I'm not a system_error.";
   };
   std::error_code actual_code;

   // When
   int actual = safe(f, actual_code);

   // Then
   ASSERT_EQ(expected, actual);
   ASSERT_EQ(expected_code, actual_code);
}

}
