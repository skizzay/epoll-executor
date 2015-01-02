// vim: sw=3 ts=3 expandtab cindent
#include "epoll_service.h"
#include "event_engine.h"
#include "event_handle.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <system_error>
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

inline void reset_errno() {
   errno = 0;
}

inline void throw_system_error(const char *what) {
   auto error = make_error_code(static_cast<std::errc>(errno));
   throw std::system_error(error, what);
}


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
   auto *handler = static_cast<epolling::event_handle*>(event.data.ptr);

   if (is_error(event.events)) {
      handler->on_error();
   }

   if (is_read(event.events)) {
      handler->on_read();
   }

   if (is_write(event.events)) {
      handler->on_write();
   }
};

}


namespace epolling {

epoll_service::epoll_service(event_engine &engine) :
   event_service(engine),
   epoll_fd(::epoll_create1(EPOLL_CLOEXEC)),
   signals(nullptr)
{
   if (epoll_fd < 0) {
      throw_system_error("Failed to create epoll file descriptor.");
   }
}


epoll_service::~epoll_service() noexcept {
   try {
      shutdown_service();
   }
   catch (const std::exception &e) {
      std::cerr << "Failed to shutdown service. " << e.what() << std::endl;
      abort();
   }
}


void epoll_service::start_monitoring(event_handle &handle, mode flags) {
   epoll_event ev = { convert_flags(flags), {&handle} };

   if (0 > ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handle.native_handle(), &ev)) {
      throw_system_error("Failed to add handle to epoll.");
   }
}


void epoll_service::stop_monitoring(event_handle &handle) {
   stop_monitoring(handle.native_handle());
}


void epoll_service::block_on_signals(const ::sigset_t *signal_set) {
   if (signals != nullptr) {
      errno = EAGAIN;
      throw_system_error("Cannot register more than one signal manager with epoll.");
   }
   signals = signal_set;
}


std::pair<std::error_code, bool> epoll_service::poll(std::size_t max_events, std::chrono::nanoseconds timeout) {
   std::vector<::epoll_event> events{max_events, ::epoll_event{0U, {nullptr}}};;
   reset_errno();
   int num_events = do_poll(epoll_fd, events.data(), events.size(),
                            std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count(),
                            signals);

   if (0 > num_events) {
      std::for_each(begin(events), begin(events) + num_events, fire_event_callbacks);
      return make_pair(std::error_code{}, true);
   }
   else if (0 == num_events) {
      return make_pair(std::error_code{}, false);
   }

   return make_pair(make_error_code(static_cast<std::errc>(errno)), false);
}


void epoll_service::close_handle(native_handle_type handle) {
   if (0 > ::close(handle)) {
      throw_system_error("Failed to close handle.");
   }
   stop_monitoring(handle);
}


void epoll_service::stop_monitoring(native_handle_type handle) {
   if (handle != epoll_fd) {
      if (0 > ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handle, nullptr)) {
         throw_system_error("Failed to unregister handle with epoll.");
      }
   }
}


void epoll_service::shutdown_service() {
   if (0 < epoll_fd) {
      close_handle(epoll_fd);
      epoll_fd = -1;
   }
}

}
