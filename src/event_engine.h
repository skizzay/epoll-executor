// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_ENGINE_H__
#define EPOLLING_EVENT_ENGINE_H__

#include "notification.h"
#include <atomic>
#include <chrono>
#include <experimental/executor>
#include <memory>

namespace epolling {

class event_service;
class signal_manager;
enum class mode;

class event_engine : public std::experimental::execution_context,
                     public std::enable_shared_from_this<event_engine> {
   friend class event_handle;
   friend class signal_manager;

public:
   //typedef std::chrono::nanoseconds duration_type;
   using duration_type = std::chrono::nanoseconds;

   static const duration_type wait_forever;

   template<class Service>
   static inline auto create(std::size_t max_events_per_poll) {
      using std::experimental::use_service;

      std::shared_ptr<event_engine> engine(new event_engine(max_events_per_poll));
      engine->service = &use_service<Service>(*engine);
      engine->wakeup = new notification(*engine, notification::behavior::conditional, 0);
      return engine;
   }
   event_engine() = delete;

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
   event_engine(std::size_t mepp);
   void set_signal_manager(signal_manager *manager);
   bool do_poll(std::size_t max_events_to_poll, duration_type timeout);
   void do_stop(std::error_code reason);
   void do_run(duration_type timeout);

   std::error_code stop_reason;
   event_service *service;
   notification *wakeup;
   const std::size_t max_events_per_poll;
   std::atomic<bool> exit_flag;
   std::atomic<bool> quitting;
   std::atomic<std::ptrdiff_t> execution_count;
};

}

#endif
