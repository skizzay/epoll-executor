// vim: sw=3 ts=3 expandtab cindent
#include "event_engine.h"
#include "event_service.h"
#include "notification.h"
#include "signal_manager.h"
#include <iostream>

using std::make_error_code;
using std::move;

namespace {

template<class T>
inline bool timeout_expired(T current_time, T stop_time, std::chrono::nanoseconds timeout) {
   return (timeout != epolling::event_engine::wait_forever) &&
          (current_time >= stop_time);
}


template<class T>
inline epolling::event_engine::duration_type time_remaining(T current_time, T stop_time, std::chrono::nanoseconds timeout) {
   return (timeout == epolling::event_engine::wait_forever) ? timeout : (stop_time - current_time);
}

}


namespace epolling {

const event_engine::duration_type event_engine::wait_forever{-1};


class event_engine::reset_for_execution {
   std::atomic<bool> &exit_flag;
   std::atomic<std::ptrdiff_t> &execution_count;
   std::error_code &stop_reason;

public:
   inline reset_for_execution(std::atomic<bool> &ef, std::atomic<std::ptrdiff_t> &ec, std::error_code &sr) :
      exit_flag(ef),
      execution_count(ec),
      stop_reason(sr)
   {
      if (++execution_count == 1) {
         exit_flag = false;
         stop_reason.clear();
      }
   }

   inline ~reset_for_execution() {
      --execution_count;
   }
};


event_engine::event_engine() :
   stop_reason(),
   wakeup(nullptr),
   service(nullptr),
   max_events_per_poll(0U),
   exit_flag(),
   quitting(),
   execution_count()
{
}


event_engine::~event_engine() noexcept {
   try {
      quitting = true;
      if (running()) {
         do_stop(make_error_code(std::errc::operation_canceled));
         while (running()) {
            std::this_thread::yield();
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
   if (!quitting) {
      do_run(timeout);
   }
   return stop_reason;
}


bool event_engine::poll(duration_type timeout) {
   return do_poll(max_events_per_poll, timeout);
}


bool event_engine::poll_one(duration_type timeout) {
   return do_poll(1U, timeout);
}


void event_engine::stop(std::error_code reason) {
   if (!quitting) {
      do_stop(move(reason));
   }
}

void event_engine::start_monitoring(event_handle &handle, mode flags) {
   service->start_monitoring(handle, flags);
}


void event_engine::stop_monitoring(event_handle &handle) {
   service->stop_monitoring(handle);
}


void event_engine::set_signal_manager(signal_manager *manager) {
   if (manager == nullptr) {
      service->block_on_signals(nullptr);
   }
   else {
      service->block_on_signals(&manager->signals);
   }
}


bool event_engine::do_poll(std::size_t max_events_to_poll, event_engine::duration_type timeout) {
   bool result = false;

   if (!quitting) {
      reset_for_execution ignored{exit_flag, execution_count, stop_reason};
      result = service->poll(max_events_to_poll, timeout).second;
      (void)ignored;
   }

   return result;
}


void event_engine::do_stop(std::error_code reason) {
   bool expected = false;
   if (exit_flag.compare_exchange_weak(expected, true, std::memory_order_acq_rel, std::memory_order_relaxed) && !expected) {
      stop_reason = move(reason);
      wakeup->set(1);
   }
}


void event_engine::do_run(duration_type timeout) {
   reset_for_execution ignored{exit_flag, execution_count, stop_reason};
   std::chrono::steady_clock::time_point current_time(std::chrono::steady_clock::now());
   std::chrono::steady_clock::time_point stop_time(current_time + timeout);

   while (!timeout_expired(current_time, stop_time, timeout) && !exit_flag.load(std::memory_order_acquire)) {
      std::error_code run_error;
      tie(run_error, std::ignore) = service->poll(max_events_per_poll, time_remaining(current_time, stop_time, timeout));
      if (run_error) {
         stop(run_error);
      }
      current_time = std::chrono::steady_clock::now();
   }
   (void)ignored;
}

}
