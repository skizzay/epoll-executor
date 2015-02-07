// vim: sw=3 ts=3 expandtab cindent
#include "epoll_service.h"
#if 0
#include "event_activation.h"
#include "event_engine.h"
#include "event_handle.h"
#include "bits/exceptions.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <iostream>

using std::begin;
using std::end;
using std::fill;
using std::make_error_code;
using std::make_pair;

namespace {

constexpr inline uint32_t set_flag(epolling::mode in_flag, epolling::mode flags, uint32_t out_flag) noexcept {
   return ((in_flag & flags) != epolling::mode::none) ? out_flag : 0U;
}


inline uint32_t convert_flags(epolling::mode in_flags) noexcept {
   using epolling::mode;

   return set_flag(mode::read, in_flags, EPOLLIN) |
          set_flag(mode::urgent_read, in_flags, EPOLLPRI) |
          set_flag(mode::write, in_flags, EPOLLOUT) |
          set_flag(mode::one_time, in_flags, EPOLLONESHOT) |
          EPOLLRDHUP | EPOLLET;
}


inline epolling::mode set_flag(uint32_t in_flag, uint32_t flags, epolling::mode out_flag) noexcept {
   return (in_flag & flags) ? out_flag : epolling::mode::none;
}


inline epolling::mode convert_flags(uint32_t in_flags) noexcept {
   using epolling::mode;
   return set_flag(EPOLLIN, in_flags, mode::read) |
          set_flag(EPOLLPRI, in_flags, mode::urgent_read) |
          set_flag(EPOLLOUT, in_flags, mode::write);
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
   auto *activation = static_cast<epolling::event_activation*>(event.data.ptr);
   assert(activation != nullptr);
   activation->callback(convert_flags(event.events), activation->object);
};

}


namespace epolling {

epoll_service::epoll_service(std::experimental::execution_context &e) :
   event_service(e),
   signals(nullptr),
   epoll_fd(::epoll_create1(EPOLL_CLOEXEC))
{
}


epoll_service::~epoll_service() noexcept {
}


void epoll_service::start_monitoring(int fd, mode flags, void (*callback)(mode, void *), void *object) {
   (void)safe([=] {
         event_activation &activation = activations.get(fd, callback, object);
         epoll_event ev = {convert_flags(flags), {&activation}};
         return ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
      }, "Failed to register handle to epoll.");
}


void epoll_service::update_monitoring(int fd, mode flags) {
   (void)safe([=] {
         event_activation &activation = activations.get(fd);
         epoll_event ev = {convert_flags(flags), {&activation}};
         return ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
      }, "Failed to modify handle with epoll.");
}


void epoll_service::stop_monitoring(int fd) {
   (void)safe([=] {
         activations.deactivate(fd);
         return ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
      }, "Failed to unregister handle with epoll.");
}


void epoll_service::block_on_signals(const ::sigset_t *signal_set) {
   details_::errno_context c;
std::cout << "signals=" << signals << ", signal_set=" << signal_set << std::endl;
   if ((signals != nullptr) && (signals != signal_set) && (signal_set != nullptr)) {
      errno = EEXIST;
      throw std::system_error(c.create_error_code(), "Cannot register more than one signal manager with epoll.");
   }
   signals = signal_set;
}


std::pair<std::error_code, bool> epoll_service::poll(std::size_t max_events, std::chrono::nanoseconds timeout) {
   std::vector<::epoll_event> events{max_events, ::epoll_event{0U, {nullptr}}};
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
      epoll_fd = handle<tag, int, -1>{};
   }
}

}
#endif
