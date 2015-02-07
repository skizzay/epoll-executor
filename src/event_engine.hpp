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

   std::atomic<bool> &exit_flag;
   std::atomic<std::ptrdiff_t> &execution_count;
   std::error_code &stop_reason;
   time_point &now;

public:
   inline reset_for_execution(std::atomic<bool> &ef, std::atomic<std::ptrdiff_t> &ec, std::error_code &sr,
                              std::atomic<EventService*> &srvc, time_point &t) noexcept :
      exit_flag(ef),
      execution_count(ec),
      stop_reason(sr),
      now(t),
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

}


template<class ES, template<class> class D>
inline event_engine<ES, D>::event_engine(std::size_t mepp) :
   std::experimental::execution_context(),
   stop_reason(),
   service(&std::experimental::use_service<ES>(*this)),
   wakeup(),
   max_events_per_poll(mepp),
   exit_flag(false),
   execution_count(0)
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
      srvc->start_monitoring<T, OnActivation>(h, flags, object);
   }
}


template<class ES, template<class> class D>
template<class Tag, class Impl, Impl Invalid>
inline void event_engine<ES, D>::update_monitoring(const handle<Tag, Impl, Invalid> &h, mode flags) {
   ES *srvc = service.load();
   if ((srvc != nullptr) && h.valid()) {
      srvc->update_monitoring(h, flags);
   }
}


template<class ES, template<class> class D>
template<class Tag, class Impl, Impl Invalid>
inline void event_engine<ES, D>::stop_monitoring(handle<Tag, Impl, Invalid> &h) {
   ES *srvc = service.load();
   if ((srvc != nullptr) && h.valid()) {
      srvc->stop_monitoring(h);
      h = {};
   }
}


template<class ES, template<class> class D>
inline void event_engine<ES, D>::quit() {
   stop(std::error_code{});
}


template<class ES, template<class> class D>
inline bool event_engine<ES, D>::running() const {
   return execution_count > 0U;
}


template<class ES, template<class> class D>
inline const typename event_engine<ES, D>::time_point &event_engine<ES, D>::time() const {
   return cached_now;
}


template<class ES, template<class> class D>
void event_engine<ES, D>::set_signal_manager(const signal_manager *manager) {
   (void) manager;
}


template<class ES, template<class> class D>
inline void event_engine<ES, D>::do_stop(ES *srvc, std::error_code reason) {
   using std::memory_order_acq_rel;
   using std::memory_order_relaxed;
   using std::move;

   if (srvc != nullptr) {
      bool expected = false;
      if (exit_flag.compare_exchange_weak(expected, true, memory_order_acq_rel, memory_order_relaxed) && !expected) {
         stop_reason = move(reason);
         wakeup->set(1);
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
template<class DurationType>
inline bool event_engine<ES, D>::do_poll(ES *srvc, std::size_t max_events_to_poll, DurationType timeout) {
   using std::ignore;
   using std::tie;

   bool events_executed = false;

   if (srvc != nullptr) {
      tie(ignore, events_executed) = srvc->poll(max_events_to_poll, timeout);
   }

   return events_executed;
}


template<class ES, template<class> class D>
template<class DurationType>
inline void event_engine<ES, D>::do_run(ES *srvc, DurationType timeout) {
   using namespace std;

   if (srvc != nullptr) {
      error_code run_error;
      chrono::steady_clock::time_point stop_time(time() + timeout);

      while (!exit_flag.load(memory_order_acquire)) {
         auto time_remaining = details_::time_remaining(time(), stop_time, timeout);
         tie(run_error, ignore) = srvc->poll(max_events_per_poll, time_remaining);
         if (run_error) {
            stop(run_error);
         }

         cached_now = chrono::steady_clock::now();
         if (details_::timeout_expired(time(), stop_time, timeout)) {
            stop(make_error_code(errc::timed_out));
         }
      }
   }
}

}

#endif
