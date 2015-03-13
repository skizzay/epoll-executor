// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_ENGINE_HPP__
#define EPOLLING_EVENT_ENGINE_HPP__

#ifndef EPOLLING_EVENT_ENGINE_H__
#  error This file must be included from event_engine.h
#endif

#include "notification.h"

namespace epolling {

namespace details_ {

using namespace std::chrono_literals;


inline bool timeout_expired(std::chrono::steady_clock::time_point current_time,
                            std::chrono::steady_clock::time_point stop_time,
                            std::chrono::nanoseconds timeout) {
   return (timeout >= 0ns) && (current_time >= stop_time);
}


inline std::chrono::nanoseconds time_remaining(std::chrono::steady_clock::time_point current_time,
                                               std::chrono::steady_clock::time_point stop_time,
                                               std::chrono::nanoseconds timeout) {
   using std::max;

   return (timeout < 0ns) ? timeout : max(0ns, (stop_time - current_time));
}


template<class EventService>
class reset_for_execution final {
   using time_point = std::chrono::steady_clock::time_point;

   std::atomic<std::ptrdiff_t> &execution_count;

public:
   inline reset_for_execution(std::atomic<bool> &exit_flag, std::atomic<std::ptrdiff_t> &ec, std::error_code &stop_reason,
                              std::atomic<EventService*> &srvc, time_point &now) noexcept :
      execution_count(ec),
      service(srvc.load())
   {
      if (service != nullptr) {
         if (++execution_count == 1) {
            exit_flag = false;
            stop_reason.clear();
         }
      }
      now = std::chrono::steady_clock::now();
   }

   inline ~reset_for_execution() noexcept {
      if (service != nullptr) {
         --execution_count;
      }
   }

   EventService *service;
};


template<class EventService, class DurationType>
class poll_service final {
   std::atomic<bool> &service_polling;
   std::size_t max_events_to_poll;
   DurationType time_remaining;
   EventService *service;
public:
   poll_service(std::atomic<bool> &sp, std::size_t mepp, DurationType tr, EventService *s) :
      service_polling(sp),
      max_events_to_poll(mepp),
      time_remaining(tr),
      service(s)
   {
      service_polling.store(true, std::memory_order::memory_order_release);
   }

   inline ~poll_service() {
      service_polling.store(false, std::memory_order::memory_order_release);
   }

   auto go() {
      return service->poll(max_events_to_poll, time_remaining);
   }
};

}


template<class ES, template<class> class D>
inline event_engine<ES, D>::event_engine(std::size_t mepp) :
   std::experimental::execution_context(),
   stop_reason(),
   service(&std::experimental::use_service<ES>(*this)),
   wakeup(),
   max_events_per_poll(mepp),
   service_polling(false),
   exit_flag(false),
   execution_count(0),
   cached_now(std::chrono::steady_clock::now())
{
   wakeup.reset(new notification(*this, notification::behavior::conditional, 0));
}


template<class ES, template<class> class D>
inline event_engine<ES, D>::~event_engine() noexcept {
   try {
      auto *srvc = service.exchange(nullptr);
      if (running()) {
         do_stop(srvc, make_error_code(std::errc::operation_canceled));
         wait_until_not_running();
      }
      wakeup.reset();
   }
   catch (...) {
      std::terminate();
   }
}


template<class ES, template<class> class D>
template<class DurationType>
std::error_code event_engine<ES, D>::run(DurationType timeout) {
   details_::reset_for_execution<ES> context{exit_flag, execution_count, stop_reason, service, cached_now};
   do_run(context.service, timeout);
   return stop_reason;
}


template<class ES, template<class> class D>
template<class DurationType>
inline bool event_engine<ES, D>::poll(DurationType timeout) {
   details_::reset_for_execution<ES> context{exit_flag, execution_count, stop_reason, service, cached_now};
   return do_poll(context.service, max_events_per_poll, timeout);
}


template<class ES, template<class> class D>
template<class DurationType>
inline bool event_engine<ES, D>::poll_one(DurationType timeout) {
   details_::reset_for_execution<ES> context{exit_flag, execution_count, stop_reason, service, cached_now};
   return do_poll(context.service, 1U, timeout);
}


template<class ES, template<class> class D>
inline void event_engine<ES, D>::stop(std::error_code reason) {
   if (running()) {
      do_stop(service.load(), move(reason));
   }
}


template<class ES, template<class> class D>
template<class T, void (T::*OnActivation)(mode), class Tag, class Impl, Impl Invalid, class U>
inline void event_engine<ES, D>::start_monitoring(const handle<Tag, Impl, Invalid> &h, mode flags, U &object) {
   ES *srvc = service.load();
   if ((srvc != nullptr) && h.valid()) {
      wait_until_woken_up();
      srvc->start_monitoring<T, OnActivation>(h, flags, object);
   }
}


template<class ES, template<class> class D>
template<class Tag, class Impl, Impl Invalid>
inline void event_engine<ES, D>::update_monitoring(const handle<Tag, Impl, Invalid> &h, mode flags) {
   ES *srvc = service.load();
   if ((srvc != nullptr) && h.valid()) {
      wait_until_woken_up();
      srvc->update_monitoring(h, flags);
   }
}


template<class ES, template<class> class D>
template<class Tag, class Impl, Impl Invalid>
inline void event_engine<ES, D>::stop_monitoring(handle<Tag, Impl, Invalid> &h) {
   ES *srvc = service.load();
   if ((srvc != nullptr) && h.valid()) {
      wait_until_woken_up();
      srvc->stop_monitoring(h);
      h = {};
   }
}


template<class ES, template<class> class D>
inline std::error_code event_engine<ES, D>::block_signal(signal_handle signum) {
   using std::this_thread::yield;

   std::error_code result{};
   ES *srvc = service.load();
   if ((srvc != nullptr) && signum.valid()) {
      wait_until_woken_up();
      result = srvc->block_signal(signum);
   }
   return result;
}


template<class ES, template<class> class D>
inline void event_engine<ES, D>::quit() {
   stop(std::error_code{});
}


template<class ES, template<class> class D>
inline bool event_engine<ES, D>::running() const {
   return execution_count > 0;
}


template<class ES, template<class> class D>
inline bool event_engine<ES, D>::polling() const {
   return service_polling.load(std::memory_order_consume);
}


template<class ES, template<class> class D>
inline const typename event_engine<ES, D>::time_point &event_engine<ES, D>::time() const {
   return cached_now;
}


template<class ES, template<class> class D>
inline void event_engine<ES, D>::do_stop(ES *srvc, std::error_code reason) {
   using std::move;

   if (srvc != nullptr) {
      bool expected = false;
      if (exit_flag.compare_exchange_weak(expected, true, std::memory_order_acq_rel, std::memory_order_relaxed) && !expected) {
         stop_reason = move(reason);
         if (polling()) {
            wakeup->set(1);
         }
      }
   }
}


template<class ES, template<class> class D>
inline void event_engine<ES, D>::wait_until_not_running() {
   using std::this_thread::yield;

   while (running()) {
      yield();
   }
}


template<class ES, template<class> class D>
inline void event_engine<ES, D>::wait_until_woken_up() {
   using std::this_thread::yield;

   if (polling()) {
      wakeup->set(1);
      while (polling()) {
         yield();
      }
   }
}


template<class ES, template<class> class D>
template<class DurationType>
inline bool event_engine<ES, D>::do_poll(ES *srvc, std::size_t max_events_to_poll, DurationType timeout) {
   using std::tie;

   bool events_executed = false;

   if (srvc != nullptr) {
      details_::poll_service<ES, DurationType> poller{service_polling, max_events_to_poll, timeout, srvc};
      tie(std::ignore, events_executed) = poller.go();
   }

   return events_executed;
}


template<class ES, template<class> class D>
template<class DurationType>
inline void event_engine<ES, D>::do_run(ES *srvc, DurationType timeout) {
   using std::make_error_code;
   using std::tie;

   if (srvc != nullptr) {
      std::error_code run_error;
      std::chrono::steady_clock::time_point stop_time(time() + timeout);

      while (!exit_flag.load(std::memory_order_acquire)) {
         auto time_remaining = details_::time_remaining(time(), stop_time, timeout);
         {
            details_::poll_service<ES, DurationType> poller{service_polling, max_events_per_poll, timeout, srvc};
            tie(run_error, std::ignore) = poller.go();
         }
         if (run_error) {
            stop(run_error);
         }

         cached_now = std::chrono::steady_clock::now();
         if (details_::timeout_expired(time(), stop_time, timeout)) {
            stop(make_error_code(std::errc::timed_out));
         }
      }
   }
}

}

#endif
