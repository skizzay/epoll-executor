// vim: sw=3 ts=3 expandtab cindent
#include "signal_manager.h"
#include <sys/signalfd.h>

namespace {

inline void throw_system_error(const char *what) {
   using std::make_error_code;

   auto error = make_error_code(static_cast<std::errc>(errno));
   throw std::system_error(error, what);
}

}


namespace epolling {

signal_manager::signal_manager(event_engine &e) :
   event_handle(e),
   signals()
{
   (void)::sigemptyset(&signals);
}


void signal_manager::monitor_signal(native_handle_type signum) {
   if (0 == ::sigismember(&signals, signum)) {
      if (0 > ::sigaddset(&signals, signum)) {
         throw_system_error("Failed to add signal to signal set.");
      }
      update_signal_handle();
   }
}


void signal_manager::update_signal_handle() {
   native_handle_type new_handle = ::signalfd(native_handle(), &signal_set(), SFD_NONBLOCK | SFD_CLOEXEC);

   if (new_handle < 0) {
      throw_system_error("Failed to create signal handle.");
   }

   register_handle(new_handle);
}

}
