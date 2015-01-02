// vim: sw=3 ts=3 expandtab cindent
#include "event_engine.h"
#include "event_service.h"
#include "signal_manager.h"


namespace epolling {

const event_engine::duration_type event_engine::wait_forever{-1};

event_engine::event_engine() :
   stop_reason(),
   service(nullptr),
   max_events_per_poll(0U),
   exit_flag(false)
{
}


std::error_code event_engine::run(duration_type timeout) {
   reset_for_execution();

   while (!exit_flag || !stop_reason) {
      std::error_code run_error;
      tie(run_error, std::ignore) = service->poll(max_events_per_poll, timeout);
      if (run_error) {
         stop(run_error);
      }
   }

   return stop_reason;
}


bool event_engine::poll(duration_type timeout) {
   reset_for_execution();
   return service->poll(max_events_per_poll, timeout).second;
}


bool event_engine::poll_one(duration_type timeout) {
   reset_for_execution();
   return service->poll(1U, timeout).second;
}


void event_engine::stop(std::error_code reason) {
   stop_reason = reason;
   exit_flag = true;
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


void event_engine::reset_for_execution() {
   exit_flag = false;
   stop_reason.clear();
}

}
