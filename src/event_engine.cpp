// vim: sw=3 ts=3 expandtab cindent
#include "event_engine.h"
#include "event_service.h"
#include "notification.h"
#include "signal_manager.h"
#include <iostream>
#include <thread>

using std::make_error_code;
using std::max;
using std::move;
using std::this_thread::yield;
using namespace std::chrono_literals;

namespace {

template<class T>
inline bool timeout_expired(T current_time, T stop_time, std::chrono::nanoseconds timeout) {
   return (timeout != epolling::event_engine::wait_forever) &&
          (current_time >= stop_time);
}


template<class T>
inline epolling::event_engine::duration_type time_remaining(T current_time, T stop_time, std::chrono::nanoseconds timeout) {
   return (timeout == epolling::event_engine::wait_forever) ? timeout : max(0ns, (stop_time - current_time));
}


class reset_for_execution {
   std::atomic<bool> &exit_flag;
   std::atomic<std::ptrdiff_t> &execution_count;
   std::error_code &stop_reason;
   epolling::event_engine::time_point &now;

public:
   inline reset_for_execution(std::atomic<bool> &ef, std::atomic<std::ptrdiff_t> &ec, std::error_code &sr,
                              std::atomic<epolling::event_service*> &srvc, epolling::event_engine::time_point &t) :
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

   inline ~reset_for_execution() {
      if (service != nullptr) {
         --execution_count;
      }
   }

   epolling::event_service *service;
};

}


namespace epolling {

const event_engine::duration_type event_engine::wait_forever = -1ns;


event_engine::event_engine(std::size_t mepp) :
   std::experimental::execution_context(),
   stop_reason(),
   service(nullptr),
   wakeup(nullptr),
   max_events_per_poll(mepp),
   exit_flag(false),
   execution_count(0)
{
}


event_engine::~event_engine() noexcept {
   try {
      auto *srvc = service.exchange(nullptr);
      if (running()) {
         do_stop(srvc, make_error_code(std::errc::operation_canceled));
         while (running()) {
            yield();
         }
      }
      delete wakeup;
   }
   catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      std::terminate();
   }
}


std::error_code event_engine::run(duration_type timeout) {
   reset_for_execution context{exit_flag, execution_count, stop_reason, service, cached_now};
   do_run(context.service, timeout);
   return stop_reason;
}


bool event_engine::poll(duration_type timeout) {
   reset_for_execution context{exit_flag, execution_count, stop_reason, service, cached_now};
   return do_poll(context.service, max_events_per_poll, timeout);
}


bool event_engine::poll_one(duration_type timeout) {
   reset_for_execution context{exit_flag, execution_count, stop_reason, service, cached_now};
   return do_poll(context.service, 1U, timeout);
}


void event_engine::stop(std::error_code reason) {
   if (running()) {
      do_stop(service.load(), move(reason));
   }
}


void event_engine::start_monitoring(event_handle &handle, mode flags) {
   event_service *srvc = service.load();
   if ((srvc != nullptr) && handle.opened()) {
      srvc->start_monitoring(handle, flags);
   }
}


void event_engine::update_monitoring(event_handle &handle, mode flags) {
   event_service *srvc = service.load();
   if ((srvc != nullptr) && handle.opened()) {
      srvc->update_monitoring(handle, flags);
   }
}


void event_engine::stop_monitoring(event_handle &handle) {
   event_service *srvc = service.load();
   if (srvc != nullptr) {
      srvc->stop_monitoring(handle);
   }
}


void event_engine::set_signal_manager(const signal_manager *manager) {
   event_service *srvc = service.load();
   if (srvc) {
      srvc->block_on_signals((manager == nullptr) ? nullptr : &manager->signals);
   }
}


bool event_engine::do_poll(event_service *srvc, std::size_t max_events_to_poll, event_engine::duration_type timeout) {
   return (srvc == nullptr) ? false : srvc->poll(max_events_to_poll, timeout).second;
}


void event_engine::do_stop(event_service *srvc, std::error_code reason) {
   if (srvc != nullptr) {
      bool expected = false;
      if (exit_flag.compare_exchange_weak(expected, true, std::memory_order_acq_rel, std::memory_order_relaxed) && !expected) {
         stop_reason = move(reason);
         wakeup->set(1);
      }
   }
}


void event_engine::do_run(event_service *srvc, duration_type timeout) {
   if (srvc != nullptr) {
      std::chrono::steady_clock::time_point stop_time(time() + timeout);

      while (!exit_flag.load(std::memory_order_acquire)) {
         std::error_code run_error;
         tie(run_error, std::ignore) = srvc->poll(max_events_per_poll, time_remaining(time(), stop_time, timeout));
         if (run_error) {
            stop(run_error);
         }

         cached_now = std::chrono::steady_clock::now();
         if (timeout_expired(time(), stop_time, timeout)) {
            stop(make_error_code(std::errc::timed_out));
         }
      }
   }
}

}
