// vim: sw=3 ts=3 expandtab cindent
#include "notification.h"
#include "event_engine.h"
#include "epoll_service.h"
#include <gtest/gtest.h>
#include <experimental/thread_pool>
#include <future>
#include <memory>

namespace {

using namespace std;
using namespace epolling;
using std::chrono::seconds;
using std::experimental::post;
using std::experimental::thread_pool;

constexpr size_t max_events_per_poll = 10;

struct notification_tests : public ::testing::Test {
   notification_tests() :
      Test(),
      engine(std::make_shared<event_engine<epoll_service>>((max_events_per_poll))),
      polling_result(),
      run_result()
   {
   }

   virtual ~notification_tests() override {
      join();
   }

   inline unique_ptr<notification> create_target() {
      return make_unique<notification>(*engine, notification::behavior::conditional, 0);
   }

   inline void run() {
      if (!polling_result.valid()) {
         polling_result = async(launch::async, [=] { return engine->run(); });
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
         result = engine->poll(chrono::seconds{0});
      }
      catch (const exception &e) {
         cerr << e.what() << endl;
      }
      return result;
   }

   inline bool try_poll_one() {
      bool result = false;
      try {
         result = engine->poll_one();
      }
      catch (const exception &e) {
         cerr << e.what() << endl;
      }
      return result;
   }

   shared_ptr<event_engine<epoll_service>> engine;
   future<error_code> polling_result;
   error_code run_result;
};


TEST_F(notification_tests, constructor) {
   run();
   const size_t num_threads = thread::hardware_concurrency();
   vector<unique_ptr<notification>> notifications;
   notifications.reserve(num_threads * 5);
   for (size_t i = 0; i < notifications.capacity(); ++i) {
      notifications.emplace_back(create_target());
   }

   thread_pool pool{num_threads};

   for (size_t i = 0; i < notifications.size(); ++i) {
      post(pool, [this, i, &notifications] {
            notifications[i]->set(i + 1);
         });
   }

   pool.join();
}

}
