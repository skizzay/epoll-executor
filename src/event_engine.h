// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_ENGINE_H__
#define EPOLLING_EVENT_ENGINE_H__

#include <experimental/executor>
#include <memory>

namespace epolling {

class event_handle;
class event_service;
class signal_manager;
enum class mode;

class event_engine : public std::experimental::execution_context {
   friend class event_handle;
   friend class signal_manager;

public:
   typedef std::chrono::nanoseconds duration_type;

   static const duration_type wait_forever;

   template<class Service>
   static inline std::unique_ptr<event_engine> create(std::size_t max_events_per_poll) {
      using std::experimental::use_service;

      auto engine = std::make_unique<event_engine>();
      engine->max_events_per_poll = max_events_per_poll;
      engine->service = use_service<Service>(*engine);
      return std::move(engine);
   }

   std::error_code run(duration_type timeout=wait_forever);
   bool poll(duration_type timeout=wait_forever);
   bool poll_one(duration_type timeout=wait_forever);
   void stop(std::error_code reason);

   inline void quit() {
      stop(std::error_code{});
   }

private:
   event_engine();

   void start_monitoring(event_handle &handle, mode flags);
   void stop_monitoring(event_handle &handle);
   void set_signal_manager(signal_manager *manager);
   void reset_for_execution();

   std::error_code stop_reason;
   event_service *service;
   std::size_t max_events_per_poll;
   bool exit_flag;
};

}

#endif
