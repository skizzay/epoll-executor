// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_ENGINE_H__
#define EPOLLING_EVENT_ENGINE_H__

#include "notification.h"
#include <experimental/executor>
#include <atomic>
#include <memory>

namespace epolling {

class event_service;
class signal_manager;
enum class mode;

class event_engine : public std::experimental::execution_context {
   class reset_for_execution;
   friend class reset_for_execution;
   friend class event_handle;
   friend class signal_manager;

public:
   typedef std::chrono::nanoseconds duration_type;

   static const duration_type wait_forever;

   template<class Service>
   static inline std::unique_ptr<event_engine> create(std::size_t max_events_per_poll) {
      using std::experimental::use_service;

      auto engine = std::unique_ptr<event_engine>(new event_engine{});
      engine->max_events_per_poll = max_events_per_poll;
      engine->service = &use_service<Service>(*engine);
      engine->wakeup = new notification(*engine, notification::behavior::conditional, 0);
      return std::move(engine);
   }

   virtual ~event_engine() noexcept final override;

   std::error_code run(duration_type timeout=wait_forever);
   bool poll(duration_type timeout=wait_forever);
   bool poll_one(duration_type timeout=wait_forever);
   void stop(std::error_code reason);

   inline void quit() {
      stop(std::error_code{});
   }

   inline bool running() const {
      return execution_count > 0U;
   }

private:
   event_engine();

   void start_monitoring(event_handle &handle, mode flags);
   void stop_monitoring(event_handle &handle);
   void set_signal_manager(signal_manager *manager);
   bool do_poll(std::size_t max_events_to_poll, duration_type timeout);

   std::error_code stop_reason;
   notification *wakeup;
   event_service *service;
   std::size_t max_events_per_poll;
   std::atomic<bool> exit_flag;
   std::atomic<std::ptrdiff_t> execution_count;
};

}

#endif
