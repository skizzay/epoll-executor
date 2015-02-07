// vim: sw=3 ts=3 expandtab cindent
#ifndef FAKE_SERVICE_H__
#define FAKE_SERVICE_H__

#include "activation.h"
#include "mode.h"
#include <experimental/executor>
#include <gmock/gmock.h>

class fake_service : public std::experimental::execution_context::service {
public:
   template<class Tag, class Impl, Impl Invalid>
   using handle = epolling::handle<Tag, Impl, Invalid>;
   using mode = epolling::mode;
   using native_handle_type = epolling::native_handle_type;

   static fake_service *singleton;

   explicit inline fake_service(std::experimental::execution_context &ec) :
      std::experimental::execution_context::service(ec)
   {
      using ::testing::An;
      using ::testing::Gt;
      using ::testing::Invoke;
      using ::testing::NotNull;

      assert(singleton == nullptr);
      singleton = this;
      EXPECT_CALL(*this, start_monitoring(Gt(2), mode::urgent_read, NotNull()))
         .Times(1);
      EXPECT_CALL(*this, shutdown_service())
         .WillOnce(Invoke(fake_service::reset_singleton));
   }

   template<class T, void (T::*)(mode), class Tag, class Impl, Impl Invalid, class U>
   void start_monitoring(const handle<Tag, Impl, Invalid> &h, mode f, U &u) {
      start_monitoring(h, f, &u);
   }

   MOCK_METHOD3(start_monitoring, void(native_handle_type, mode, void *));
   MOCK_METHOD2(update_monitoring, void(native_handle_type, mode));
   MOCK_METHOD1(stop_monitoring, void(native_handle_type));
   MOCK_METHOD1(block_on_signals, void(const ::sigset_t *));
   MOCK_METHOD2(poll, std::pair<std::error_code, bool>(std::size_t, std::chrono::nanoseconds));
   MOCK_METHOD0(shutdown_service, void());

private:
   static inline void reset_singleton() {
      singleton = nullptr;
   }
};

#endif
