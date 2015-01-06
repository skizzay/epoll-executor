// vim: sw=3 ts=3 expandtab cindent
#include "signal_manager.h"
#include "event_engine.h"
#include "bits/exceptions.h"
#include <sys/signalfd.h>

namespace {

inline ::sigset_t create_signal_set() {
   ::sigset_t signals;
   (void)::sigemptyset(&signals);
   return signals;
}


inline epolling::native_handle_type create_native_handle(epolling::native_handle_type current_fd, const ::sigset_t signals) {
   return ::signalfd(current_fd, &signals, SFD_NONBLOCK | SFD_CLOEXEC);
}

}


namespace epolling {

signal_manager::signal_manager(event_engine &e) :
   signals(create_signal_set()),
   handle(e, create_native_handle(-1, signals), mode::urgent_read),
   handlers()
{
   handle.on_read([=] {
         ::signalfd_siginfo signal_info;
         (void)handle.read(&signal_info, sizeof(signal_info));
         auto &handler = handlers[signal_info.ssi_signo];
         if (handler != nullptr) {
            handler->handle(signal_info);
         }
      });
   e.set_signal_manager(this);
}


signal_manager::~signal_manager() {
   handle.engine().set_signal_manager(nullptr);
}


void signal_manager::monitor_signal(native_handle_type signum) {
   if (0 == ::sigismember(&signals, signum)) {
      (void)safe([=] { return ::sigaddset(&signals, signum); },
                 "Failed to add signal to signal set.");
      update_signal_handle();
   }
}


void signal_manager::update_signal_handle() {
   auto new_handle = safe([=] { return create_native_handle(handle.native_handle(), signals); },
                          "Failed to create signal handle.");

   if (new_handle != handle.native_handle()) {
      handle = event_handle{handle.engine(), new_handle, mode::urgent_read};
   }
}

}
