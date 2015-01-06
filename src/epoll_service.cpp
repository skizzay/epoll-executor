// vim: sw=3 ts=3 expandtab cindent
#include "epoll_service.h"
#include "event_engine.h"
#include "event_handle.h"
#include "bits/exceptions.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>

using std::begin;
using std::end;
using std::fill;
using std::make_error_code;
using std::make_pair;

namespace {

inline uint32_t set_flag(epolling::mode in_flag, epolling::mode flags, uint32_t out_flag) {
   return (static_cast<uint32_t>(in_flag) & static_cast<uint32_t>(flags)) ? out_flag : 0;
}


inline uint32_t convert_flags(epolling::mode in_flags) {
   using epolling::mode;

   return set_flag(mode::read, in_flags, EPOLLIN) |
          set_flag(mode::urgent_read, in_flags, EPOLLPRI) |
          set_flag(mode::write, in_flags, EPOLLOUT) |
          set_flag(mode::one_time, in_flags, EPOLLONESHOT) |
          EPOLLRDHUP | EPOLLET;
}


inline int do_poll(int epoll_fd, ::epoll_event *events, int max_events, int timeout, const ::sigset_t *signals) {
   if (signals == nullptr) {
      return ::epoll_wait(epoll_fd, events, max_events, timeout);
   }

   return ::epoll_pwait(epoll_fd, events, max_events, timeout, signals);
}


constexpr bool is_read(uint32_t flags) {
   return (flags & (EPOLLIN | EPOLLPRI)) > 0U;
}


constexpr bool is_write(uint32_t flags) {
   return (flags & EPOLLOUT) > 0U;
}


constexpr bool is_error(uint32_t flags) {
   return (flags & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) > 0U;
}

auto fire_event_callbacks = [](::epoll_event &event) {
#if 1
   auto *handler = static_cast<epolling::event_handle*>(event.data.ptr);

   if (is_error(event.events)) {
      handler->on_error();
   }

   if (is_write(event.events)) {
      handler->on_write();
   }

   if (is_read(event.events)) {
      handler->on_read();
   }
#else
   (void)event;
#endif
};

}


namespace epolling {

epoll_service::epoll_service(std::experimental::execution_context &e) :
   event_service(e),
   epoll_fd(-1),
   signals(nullptr)
{
   epoll_fd = safe([]{ return ::epoll_create1(EPOLL_CLOEXEC); },
                   "Failed to create epoll file descriptor.");
}


epoll_service::~epoll_service() noexcept {
}


void epoll_service::start_monitoring(event_handle &handle, mode flags) {
   (void)safe([&handle, flags, this] {
         epoll_event ev = {convert_flags(flags), {&handle}};
         return ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handle.native_handle(), &ev);
      }, "Failed to register handle to epoll.");
}


void epoll_service::stop_monitoring(event_handle &handle) {
   (void)safe([&handle, this] { return ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handle.native_handle(), nullptr); },
              "Failed to unregister handle with epoll.");
}


void epoll_service::block_on_signals(const ::sigset_t *signal_set) {
   if ((signals != nullptr) && (signals != signal_set) && (signal_set != nullptr)) {
      errno = EAGAIN;
      throw_system_error("Cannot register more than one signal manager with epoll.");
   }
   signals = signal_set;
}


std::pair<std::error_code, bool> epoll_service::poll(std::size_t max_events, std::chrono::nanoseconds timeout) {
   std::vector<::epoll_event> events{max_events, ::epoll_event{0U, {nullptr}}};
   reset_errno();
   int num_events = do_poll(epoll_fd, events.data(), events.size(),
                            std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count(),
                            signals);

   if (0 < num_events) {
      std::for_each(begin(events), begin(events) + num_events, fire_event_callbacks);
      return make_pair(std::error_code{}, true);
   }
   else if (0 == num_events) {
      return make_pair(std::error_code{}, false);
   }

   return make_pair(make_error_code(static_cast<std::errc>(errno)), false);
}


void epoll_service::shutdown_service() {
   if (0 < epoll_fd) {
      (void)safe([=] { return ::close(epoll_fd); }, "Failed to close epoll handle.");
      epoll_fd = -1;
   }
}

}
