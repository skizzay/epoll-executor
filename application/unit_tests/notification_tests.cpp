// vim: sw=3 ts=3 expandtab cindent
#include "notification.h"
#include "event_engine.h"
#include "epoll_service.h"
#include <gtest/gtest.h>
#include <experimental/thread_pool>
#include <future>
#include <memory>

namespace {

using namespace epolling;
using std::make_unique;

constexpr std::size_t max_events_per_poll = 10;

struct notification_tests : public ::testing::Test {
   notification_tests() :
      Test(),
      engine(event_engine::create<epoll_service>(max_events_per_poll)),
      polling_result(),
      run_result()
   {
   }

   virtual ~notification_tests() override {
      join();
   }

   inline std::unique_ptr<notification> create_target() {
      return make_unique<notification>(*engine, notification::behavior::conditional, 0);
   }

   inline void run() {
      if (!polling_result.valid()) {
         polling_result = std::async(std::launch::async, [=] { return engine->run(); });
      }
   }

   inline void join() {
      if (polling_result.valid()) {
         engine->quit();
         run_result = polling_result.get();
      }
   }

   inline bool try_poll() {
      bool result = false;
      try {
         result = engine->poll(std::chrono::seconds{0});
      }
      catch (const std::exception &e) {
         std::cerr << e.what() << std::endl;
      }
      return result;
   }

   inline bool try_poll_one() {
      bool result = false;
      try {
         result = engine->poll_one();
      }
      catch (const std::exception &e) {
         std::cerr << e.what() << std::endl;
      }
      return result;
   }

   std::unique_ptr<event_engine> engine;
   std::future<std::error_code> polling_result;
   std::error_code run_result;
};


TEST_F(notification_tests, constructor) {
   run();
   const std::size_t num_threads = std::thread::hardware_concurrency();
   std::vector<std::unique_ptr<notification>> notifications;
   notifications.reserve(num_threads * 2);
   for (std::size_t i = 0; i < notifications.capacity(); ++i) {
      notifications.emplace_back(create_target());
   }

#if 0
   std::vector<std::thread> threads;
   threads.reserve(num_threads);
   for (std::size_t i = 0; i < num_threads; ++i) {
      threads.emplace_back([this, i, &notifications] {
            notifications[i]->set(10 + (i + 1));
            try_poll_one();
            std::ostringstream out;
            out << "Notification(i=" << i << ") = " << notifications[i]->get() << '\n';
            std::cout << out.str() << std::flush;
         });
   }

   for (auto &thread : threads) {
      thread.join();
   }
#else
   std::experimental::thread_pool pool;

   for (std::size_t i = 0; i < notifications.size(); ++i) {
      std::experimental::post(pool, [this, i, &notifications] {
            notifications[i]->set(i + 1);
         });
   }

   pool.join();
#endif

#if 0
   for (std::size_t i = 0; i < notifications.size(); ++i) {
      std::cout << "Notification(i=" << i << ") = " << notifications[i]->get() << std::endl;
   }
#endif
}

}
