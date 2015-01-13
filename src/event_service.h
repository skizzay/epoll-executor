// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_SERVICE_H__
#define EPOLLING_EVENT_SERVICE_H__

#include <experimental/executor>
#include <chrono>
#include <csignal>

namespace epolling {

class event_handle;
enum class mode : int;

typedef int native_handle_type;

class event_service : public std::experimental::execution_context::service {
public:
   virtual ~event_service() = default;

   virtual void start_monitoring(event_handle &handle, mode flags) = 0;
   virtual void update_monitoring(event_handle &handle, mode flags) = 0;
   virtual void stop_monitoring(event_handle &handle) = 0;
   virtual void block_on_signals(const ::sigset_t *signals) = 0;
   virtual std::pair<std::error_code, bool> poll(std::size_t max_events, std::chrono::nanoseconds timeout) = 0;

protected:
   explicit event_service(std::experimental::execution_context &);
};

}

#endif
