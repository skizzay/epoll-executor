// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EPOLL_SERVICE_H__
#define EPOLLING_EPOLL_SERVICE_H__

#include "event_service.h"
#include <atomic>
#include <csignal>

namespace epolling {

class epoll_service : public event_service {
public:
   explicit epoll_service(std::experimental::execution_context &e);
   virtual ~epoll_service() noexcept final override;

   virtual void start_monitoring(event_handle &handle) final override;
   virtual void update_monitoring(event_handle &handle) final override;
   virtual void stop_monitoring(event_handle &handle) final override;
   virtual void block_on_signals(const ::sigset_t *signals) final override;
   virtual std::pair<std::error_code, bool> poll(std::size_t max_events, std::chrono::nanoseconds timeout) final override;


private:
   virtual void shutdown_service() final override;

   native_handle_type epoll_fd;
   const ::sigset_t *signals;
};

}

#endif
