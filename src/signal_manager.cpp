// vim: sw=3 ts=3 expandtab cindent
#include "signal_manager.h"
#include "event_engine.h"
#include "event_service.h"
#include "bits/exceptions.h"
#include <cassert>
#include <sys/signalfd.h>

using std::move;

namespace {

inline ::sigset_t create_signal_set() {
   ::sigset_t signals;
   (void)::sigemptyset(&signals);
   return signals;
}


inline epolling::native_handle_type create_native_handle(epolling::native_handle_type current_fd, const ::sigset_t &signals) {
   return ::signalfd(current_fd, &signals, SFD_NONBLOCK | SFD_CLOEXEC);
}

}


namespace epolling {

signal_manager::signal_manager(event_engine &e) :
   signals(create_signal_set()),
   engine(e.shared_from_this()),
   handlers(),
   handle([this] (auto) { this->on_signal_triggered(); })
{
   register_with_engine(this);
}


signal_manager::~signal_manager() {
   register_with_engine(nullptr);
}


void signal_manager::monitor_signal(native_handle_type signum) {
   if (0 == ::sigismember(&signals, signum)) {
      (void)safe([=] { return ::sigaddset(&signals, signum); },
                 "Failed to add signal to signal set.");
      register_with_engine(this);
   }
}


void signal_manager::register_with_engine(const signal_manager *self) {
   auto e = engine.lock();
   if (e != nullptr) {
      if (handle.opened()) {
         e->stop_monitoring(handle);
      }

      if (self != nullptr) {
         handle.open(create_native_handle(handle.native_handle(), signals), "Failed to create signal handle");
         e->start_monitoring(handle, mode::urgent_read);
      }

      e->set_signal_manager(self);
   }
}


void signal_manager::on_signal_triggered() {
   std::error_code error;
   ::signalfd_siginfo signal_info;
   auto num_bytes = handle.read(&signal_info, sizeof(signal_info), error);
   auto &handler = handlers[signal_info.ssi_signo];

   assert(num_bytes > 0);
   assert(!error);
   assert(handler != nullptr);

   handler->handle(signal_info);
}

}
