// vim: sw=3 ts=3 expandtab cindent
#include "event_engine.h"
#include "fake_service.h"
#include "epoll_service.h"
#include "print_to.h"
#include <gmock/gmock.h>
#include <chrono>
#include <future>
#include <memory>
#include <system_error>

namespace {

using namespace epolling;
using namespace std::chrono_literals;
using ::testing::A;
using ::testing::An;
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Mock;
using ::testing::NotNull;
using ::testing::Ref;
using ::testing::Return;
using std::async;
using std::make_error_code;
using std::make_pair;
using std::rand;
using std::this_thread::yield;


struct testing_tag {};

typedef handle<testing_tag, native_handle_type, -1> testing_handle_type;


// This isn't a valid service because there isn't a constructor that takes an execution context.
struct not_a_valid_service final : std::experimental::execution_context::service {
   template<class T, void (T::*OnActivation)(mode), class U>
   void start_monitoring(native_handle_type h, mode f, U &u) {
      (void)h;
      (void)f;
      (void)u;
   }

   void update_monitoring(native_handle_type h, mode m) {
      (void)h;
      (void)m;
   }

   void stop_monitoring(event_handle &handle) {
      (void)handle;
   }

   void block_on_signals(const ::sigset_t *signals) {
      (void)signals;
   }

   std::pair<std::error_code, bool> poll(std::size_t max_events, std::chrono::nanoseconds timeout) {
      (void)max_events;
      (void)timeout;
      return make_pair(std::error_code{}, bool{});
   }

   virtual void shutdown_service() final override {
   }
};


struct event_engine_tests : ::testing::Test {
   event_engine_tests() :
      ::testing::Test(),
      max_events_per_poll(50),
      activated(false),
      activation_flags()
   {
   }

   template<class Service>
   inline auto create_target() {
      return std::make_shared<event_engine<Service>>(max_events_per_poll);
   }

   void on_activation(mode flags) {
      activated = true;
      activation_flags = flags;
   }

   std::size_t max_events_per_poll;
   bool activated;
   mode activation_flags;
};


TEST_F(event_engine_tests, create_with_valid_event_service_should_use_the_service) {
   // Arrange, Act
   auto target = create_target<fake_service>();

   // Assert
   ASSERT_NE(nullptr, fake_service::singleton);

   (void)target;
}


TEST_F(event_engine_tests, create_with_invalid_type_should_throw_bad_cast) {
   // Arrange, Act, Assert
   ASSERT_THROW(create_target<not_a_valid_service>(), std::bad_cast);
}


TEST_F(event_engine_tests, destructor_when_invoked_should_call_system_shutdown_on_service) {
   // Arrange, Act
   {
      // there's already an expectation registered for the shutdown
      (void)create_target<fake_service>();
   }

   // Assert
   ASSERT_EQ(nullptr, fake_service::singleton);
}


TEST_F(event_engine_tests, poll_given_a_timeout_should_pass_timeout_and_max_events_per_poll_to_service) {
   // Arrange
   max_events_per_poll = static_cast<std::size_t>(rand() % 20);
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("max_events_per_poll", max_events_per_poll);
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, timeout))
      .WillOnce(Return(make_pair(std::error_code{}, bool{})));

   // Act
   (void)target->poll(timeout);

   // Assert
   // On mock expiration
}


TEST_F(event_engine_tests, poll_given_events_executed_should_return_true) {
   // Arrange
   max_events_per_poll = static_cast<std::size_t>(rand() % 20);
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("max_events_per_poll", max_events_per_poll);
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, timeout))
      .WillOnce(Return(make_pair(std::error_code{}, true)));

   // Act
   bool actual = target->poll(timeout);

   // Assert
   ASSERT_TRUE(actual);
}


TEST_F(event_engine_tests, poll_given_events_not_executed_should_return_false) {
   // Arrange
   max_events_per_poll = static_cast<std::size_t>(rand() % 20);
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("max_events_per_poll", max_events_per_poll);
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, timeout))
      .WillOnce(Return(make_pair(std::error_code{}, false)));

   // Act
   bool actual = target->poll(timeout);

   // Assert
   ASSERT_FALSE(actual);
}


TEST_F(event_engine_tests, poll_one_given_a_timeout_should_pass_timeout_and_max_events_per_poll_to_service) {
   // Arrange
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(1, timeout))
      .WillOnce(Return(make_pair(std::error_code{}, bool{})));

   // Act
   (void)target->poll_one(timeout);

   // Assert
   // On mock expiration
}


TEST_F(event_engine_tests, poll_one_given_events_executed_should_return_true) {
   // Arrange
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(1, timeout))
      .WillOnce(Return(make_pair(std::error_code{}, true)));

   // Act
   bool actual = target->poll_one(timeout);

   // Assert
   ASSERT_TRUE(actual);
}


TEST_F(event_engine_tests, poll_one_given_events_not_executed_should_return_false) {
   // Arrange
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(1, timeout))
      .WillOnce(Return(make_pair(std::error_code{}, false)));

   // Act
   bool actual = target->poll_one(timeout);

   // Assert
   ASSERT_FALSE(actual);
}


TEST_F(event_engine_tests, run_given_timeout_should_return_reason_for_stopping) {
   // Arrange
   std::error_code expected = make_error_code(static_cast<std::errc>((rand() % 4) + 1));
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, timeout))
      .WillOnce(InvokeWithoutArgs([&target, expected] () mutable {
               target->stop(expected);
               return make_pair(std::error_code{}, true);
            }));

   // Act
   auto actual = target->run(timeout);

   // Assert
   ASSERT_EQ(expected, actual);
}


TEST_F(event_engine_tests, run_when_polling_the_service_gives_an_error_should_return_error_code) {
   // Arrange
   std::error_code expected = make_error_code(static_cast<std::errc>((rand() % 4) + 1));
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, timeout))
      .WillOnce(InvokeWithoutArgs([&target, expected] () mutable {
               return make_pair(expected, true);
            }));

   // Act
   auto actual = target->run(timeout);

   // Assert
   ASSERT_EQ(expected, actual);
}


TEST_F(event_engine_tests, run_given_0ns_timeout_should_return_timed_out_reason_for_stopping) {
   // Arrange
   std::error_code expected = make_error_code(std::errc::timed_out);
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, 0ns))
      .WillOnce(InvokeWithoutArgs([&target, expected] () mutable {
               yield();
               return make_pair(std::error_code{}, true);
            }));

   // Act
   auto actual = target->run(0ns);

   // Assert
   ASSERT_EQ(expected, actual);
}


#if 1
TEST_F(event_engine_tests, run_after_engine_has_been_destructed_should_return_operation_canceled_reason_for_stopping) {
   // Arrange
   std::error_code expected = make_error_code(std::errc::operation_canceled);
   std::error_code actual;
   std::future<std::error_code> future_code;
   {
      auto target = create_target<epoll_service>();

      // Act
      future_code = async(std::launch::async, [&] { return target->run(); });
      while (!target->running()) {
         yield();
      }
   }
   actual = future_code.get();

   // Assert
   ASSERT_EQ(expected, actual);
}
#endif


TEST_F(event_engine_tests, running_after_calling_run_should_return_true) {
   // Arrange
   std::error_code expected = make_error_code(static_cast<std::errc>((rand() % 4) + 1));
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   bool actual = false;
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, timeout))
      .WillOnce(InvokeWithoutArgs([&target, &actual, expected] () mutable {
               actual = target->running();
               target->stop(expected);
               return make_pair(std::error_code{}, true);
            }));

   // Act
   (void)target->run(timeout);

   // Assert
   ASSERT_TRUE(actual);
}


TEST_F(event_engine_tests, running_after_stopping_should_return_false) {
   // Arrange
   std::error_code expected = make_error_code(static_cast<std::errc>((rand() % 4) + 1));
   std::chrono::nanoseconds timeout{rand() % 35 + 10};
   RecordProperty("timeout", timeout.count());
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, poll(max_events_per_poll, timeout))
      .WillOnce(InvokeWithoutArgs([&target, expected] () mutable {
               target->stop(expected);
               return make_pair(std::error_code{}, true);
            }));
   (void)target->run(timeout);

   // Act
   bool actual = target->running();

   // Assert
   ASSERT_FALSE(actual);
}


TEST_F(event_engine_tests, start_monitoring_given_valid_handle_and_flags_should_register_handle_with_service) {
   // Arrange
   mode expected_mode = mode::read;
   auto target = create_target<fake_service>();
   handle<testing_tag, int, -1> fd{::dup(0)};
   EXPECT_CALL(*fake_service::singleton, start_monitoring(static_cast<int>(fd), expected_mode, NotNull()))
      .Times(1);

   // Act
   target->template start_monitoring<event_engine_tests, &event_engine_tests::on_activation>(fd, expected_mode, *this);

   // Assert
   // On mock expiration
}


TEST_F(event_engine_tests, update_monitoring_given_valid_handle_and_flags_should_update_handle_with_service) {
   // Arrange
   mode expected_mode = mode::read;
   handle<testing_tag, int, -1> fd{::dup(0)};
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, update_monitoring(static_cast<int>(fd), expected_mode))
      .Times(1);

   // Act
   target->update_monitoring(fd, expected_mode);

   // Assert
   // On mock expiration
}


TEST_F(event_engine_tests, stop_monitoring_given_valid_handle_and_flags_should_register_handle_with_service_and_reset_handle) {
   // Arrange
   handle<testing_tag, int, -1> fd{::dup(0)};
   auto target = create_target<fake_service>();
   EXPECT_CALL(*fake_service::singleton, stop_monitoring(static_cast<int>(fd)))
      .Times(1);

   // Act
   target->stop_monitoring(fd);

   // Assert
   ASSERT_FALSE(fd.valid());
}

}
