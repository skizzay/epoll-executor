// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EPOLL_SERVICE_H__
#define EPOLLING_EPOLL_SERVICE_H__

#include "activation.h"
#include "mode.h"
#include "bits/exceptions.h"
#include "signal_handle.h"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <experimental/executor>
#include <sys/epoll.h>
#include <unistd.h>

namespace epolling {

using event_activation = basic_activation<void(mode)>;

namespace details_ {

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


inline int do_poll(int epoll_fd, ::epoll_event *events, int max_events, int timeout, const ::sigset_t &blocked_signals) {
   assert(events != nullptr);
   assert(max_events >= 0);

   return ::epoll_pwait(epoll_fd, events, max_events, timeout, &blocked_signals);
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
   activation->execute(convert_flags(event.events));
};

}


class epoll_service : public std::experimental::execution_context::service {
public:
   constexpr static int InvalidFileDescriptor = -1;

   explicit epoll_service(std::experimental::execution_context &e) :
      std::experimental::execution_context::service(e),
      blocked_signals(),
      epoll_fd(::epoll_create1(EPOLL_CLOEXEC))
   {
      safe([=] { return ::sigemptyset(&blocked_signals); },
           "Failed to create an empty signal set.");
   }

   virtual ~epoll_service() noexcept final override {
   }

   template<class T, void (T::*OnActivation)(mode), class Tag, class U>
   inline auto start_monitoring(const handle<Tag, int, InvalidFileDescriptor> &fd, mode flags, U &object) noexcept {
      std::error_code ec;
      safe([=, &object] {
            event_activation &activation = activations.get(fd);
            activation = event_activation_store::create_activation<T, OnActivation>(object);
            epoll_event ev = {details_::convert_flags(flags), {&activation}};
            return ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
         }, ec);
      return ec;
   }

   template<class Tag>
   inline auto update_monitoring(const handle<Tag, int, InvalidFileDescriptor> &fd, mode flags) noexcept {
      std::error_code ec;
      safe([=] {
            event_activation &activation = activations.get(fd);
            epoll_event ev = {details_::convert_flags(flags), {&activation}};
            return ::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
         }, ec);
      return ec;
   }

   template<class Tag>
   inline auto stop_monitoring(const handle<Tag, int, InvalidFileDescriptor> &fd) noexcept {
      std::error_code ec;
      safe([=] {
            activations.deactivate(fd);
            return ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
         }, ec);
      return ec;
   }

   inline std::pair<std::error_code, bool> poll(std::size_t max_events, std::chrono::nanoseconds timeout) {
      using std::begin;
      using std::end;
      using std::for_each;
      using std::make_pair;
      using std::chrono::duration_cast;

      std::vector<::epoll_event> events{max_events, {0U, {nullptr}}};
      int num_events = details_::do_poll(epoll_fd, events.data(), events.size(),
                                         duration_cast<std::chrono::milliseconds>(timeout).count(),
                                         blocked_signals);

      if (0 < num_events) {
         for_each(begin(events), begin(events) + num_events, details_::fire_event_callbacks);
         return make_pair(std::error_code{}, true);
      }
      else if (0 == num_events) {
         return make_pair(std::error_code{}, false);
      }

      return make_pair(make_error_code(static_cast<std::errc>(errno)), false);
   }

   inline std::error_code block_signal(const signal_handle &signum) {
      return add_to_signals(blocked_signals, signum);
   }


private:
   struct eventfd_tag {};

   using event_activation_store = basic_activation_store<void(mode), int>;

   static inline std::error_code add_to_signals(::sigset_t &signals, const signal_handle &signum) {
      std::error_code result;
      (void)safe([&signals, signum] { return ::sigaddset(&signals, signum); }, result);
      return result;
   }

   virtual void shutdown_service() final override {
      if (epoll_fd.valid()) {
         (void)safe([=] {
               return ::close(epoll_fd);
            }, "Failed to close epoll handle.");
         epoll_fd = {};
      }
   }

   event_activation_store activations;
   ::sigset_t blocked_signals;
   handle<eventfd_tag, int, -1> epoll_fd;
};

}

#endif
