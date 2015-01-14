// vim: sw=3 ts=3 expandtab cindent
#include "signal_manager.h"
#include "event_engine.h"
#include "event_service.h"
#include "bits/exceptions.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sys/signalfd.h>

using std::cerr;
using std::endl;
using std::errc;
using std::error_code;
using std::make_error_code;
using std::move;
using std::system_error;
using std::terminate;
using std::tie;

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
   auto error = register_with_engine(this);
   if (error) {
      throw system_error(error, "Failed to register signal manager with event engine");
   }
}


signal_manager::~signal_manager() noexcept {
   auto error = register_with_engine(nullptr);
   if (error) {
      cerr << "Failed to unregister signal manager with event engine: " << error.message() << endl;
      terminate();
   }
}


error_code signal_manager::monitor_signal(native_handle_type signum) noexcept {
   error_code error;
   if (0 == ::sigismember(&signals, signum)) {
      (void)safe([=] { return ::sigaddset(&signals, signum); }, error);
      if (!error) {
         error = register_with_engine(this);
      }
   }
   return error;
}


error_code signal_manager::register_with_engine(const signal_manager *self) noexcept {
   error_code error;
   auto e = engine.lock();
   if (e != nullptr) {
      if (handle.opened()) {
         e->stop_monitoring(handle);
      }

      if (self != nullptr) {
         error = handle.open(create_native_handle(handle.native_handle(), signals));
         if (!error) {
            e->start_monitoring(handle, mode::urgent_read);
         }
      }

      if (!error) {
         e->set_signal_manager(self);
      }
   }
   else {
      error = make_error_code(errc::identifier_removed);
   }
   return error;
}


void signal_manager::on_signal_triggered() {
   error_code error;
   int num_bytes = 0;
   ::signalfd_siginfo signal_info;
   tie(num_bytes, error) = handle.read(&signal_info, sizeof(signal_info));
   if (!error) {
      auto &handler = handlers[signal_info.ssi_signo];

      assert(num_bytes > 0);
      assert(!error);
      assert(handler != nullptr);

      handler->handle(signal_info);
   }
}

}
