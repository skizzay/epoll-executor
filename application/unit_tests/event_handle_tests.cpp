// vim: sw=3 ts=3 expandtab cindent
#include "event_handle.h"
#include <gtest/gtest.h>

namespace {

using namespace epolling;
using ::testing::Test;
using std::forward;
using std::get;
using std::move;
using std::experimental::string_view;
using std::string;
using std::tie;
using std::size_t;


struct event_handle_test : Test {
   virtual ~event_handle_test() {
      if (!filename.empty()) {
         (void)::unlink(filename.c_str());
      }
   }

   template<class F>
   inline event_handle create_target(F &&f) {
      return event_handle{forward<F>(f)};
   }

   inline int make_temp_file() {
      filename = current_test_info()->test_case_name();
      filename += ".XXXXXX";
      return ::mkstemp(&filename[0]);
   }

   static inline const ::testing::TestInfo* current_test_info() {
      return ::testing::UnitTest::GetInstance()->current_test_info();
   }

   std::string filename;
};


TEST_F(event_handle_test, swap_given_valid_handle_should_swap_settings) {
   // Given
   auto target1 = create_target([] (auto) {});
   auto target2 = create_target([] (auto) {});;
   auto target1_handle = ::dup(0);
   auto target2_handle = ::dup(0);
   target1.open(target1_handle);
   target2.open(target2_handle);

   // When
   swap(target1, target2);

   // Then
   ASSERT_EQ(target1_handle, target2.native_handle());
   ASSERT_EQ(target2_handle, target1.native_handle());
   // TODO: Figure out to test for callbacks being swapped
}


TEST_F(event_handle_test, move_constructor_given_moved_handle_should_set_handle_to_empty_state) {
   // Given
   auto target = create_target([] (auto) {});
   auto target_handle = ::dup(0);
   target.open(target_handle);

   // When
   event_handle other{move(target)};

   // Then
   ASSERT_EQ(target_handle, other.native_handle());
   ASSERT_EQ(-1, target.native_handle());
}


TEST_F(event_handle_test, move_assignment_given_moved_handle_should_set_handle_to_empty_state) {
   // Given
   auto target = create_target([] (auto) {});
   auto other = create_target([] (auto) {});
   auto target_handle = ::dup(0);
   target.open(target_handle);

   // When
   other = move(target);

   // Then
   ASSERT_EQ(target_handle, other.native_handle());
   ASSERT_EQ(-1, target.native_handle());
}


TEST_F(event_handle_test, opened_given_unopened_handle_should_return_false) {
   // Given
   auto target = create_target([] (auto) {});

   // When
   bool actual = target.opened();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(event_handle_test, opened_given_opened_handle_should_return_true) {
   // Given
   auto target = create_target([] (auto) {});
   target.open(::dup(0));

   // When
   bool actual = target.opened();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_handle_test, opened_after_opened_handle_was_closed_should_return_false) {
   // Given
   auto target = create_target([] (auto) {});
   target.open(::dup(0));
   target.close();

   // When
   bool actual = target.opened();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(event_handle_test, closed_given_unopened_handle_should_return_true) {
   // Given
   auto target = create_target([] (auto) {});

   // When
   bool actual = target.closed();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_handle_test, closed_given_opened_handle_should_return_false) {
   // Given
   auto target = create_target([] (auto) {});
   target.open(::dup(0));

   // When
   bool actual = target.closed();

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(event_handle_test, closed_after_opened_handle_was_closed_should_return_true) {
   // Given
   auto target = create_target([] (auto) {});
   target.open(::dup(0));
   target.close();

   // When
   bool actual = target.closed();

   // Then
   ASSERT_TRUE(actual);
}


TEST_F(event_handle_test, write_given_an_unopened_handle_should_return_negative_1_and_set_code_to_errno) {
   // Given
   auto target = create_target([] (auto) {});
   int data = rand();
   std::error_code actual_code;
   int actual_result = 0;

   // When
   tie(actual_result, actual_code) = target.write(&data, sizeof(data));

   // Then
   ASSERT_EQ(-1, actual_result);
   ASSERT_NE(std::error_code{}, actual_code);
}


TEST_F(event_handle_test, write_given_an_opened_handle_should_return_number_of_bytes_written) {
   // Given
   auto target = create_target([] (auto) {});
   string_view data = "handle rocks!\n";
   std::error_code actual_code;
   int actual_result = 0;
   target.open(make_temp_file());

   // When
   tie(actual_result, actual_code) = target.write(&data, data.size());

   // Then
   ASSERT_EQ(static_cast<int>(data.size()), actual_result);
}


TEST_F(event_handle_test, bytes_available_given_data_present_should_return_the_number_of_bytes_available) {
   // Given
   auto target = create_target([] (auto) {});
   string_view data = "handle rocks!\n";
   target.open(make_temp_file());
   target.write(&data, data.size());
   (void)lseek(target.native_handle(), 0, SEEK_SET);

   // When
   size_t actual = get<size_t>(target.bytes_available());

   // Then
   ASSERT_EQ(data.size(), actual);
}


TEST_F(event_handle_test, is_blocking_after_made_non_blocking_should_return_false) {
   // Given
   auto target = create_target([] (auto) {});
   target.open(make_temp_file());
   target.make_blocking();
   target.make_non_blocking();

   // When
   auto actual = get<bool>(target.is_blocking());

   // Then
   ASSERT_FALSE(actual);
}


TEST_F(event_handle_test, is_blocking_after_made_blocking_should_return_true) {
   // Given
   auto target = create_target([] (auto) {});
   target.open(make_temp_file());
   target.make_non_blocking();
   target.make_blocking();

   // When
   auto actual = get<bool>(target.is_blocking());

   // Then
   ASSERT_TRUE(actual);
}

}
