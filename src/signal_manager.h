// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_SIGNAL_MANAGER_H__
#define EPOLLING_SIGNAL_MANAGER_H__

#include "event_handle.h"
#include <array>
#include <climits>
#include <csignal>
#include <memory>

struct signalfd_siginfo;


namespace epolling {

class signal_manager : public event_handle {
   friend class event_engine;

   class signal_handler {
   public:
      virtual void handle(const ::signalfd_siginfo &signal_info) = 0;
   };

public:
   explicit signal_manager(event_engine &e);

   template<class OnSignal>
   inline void on_signal(native_handle_type signum, OnSignal &&callback);

private:
   ::sigset_t signals;
   std::array<std::unique_ptr<signal_handler>, sizeof(::sigset_t) * CHAR_BIT> handlers;

   void monitor_signal(native_handle_type signum);
   void update_signal_handle();
};


template<class OnSignal>
inline void signal_manager::on_signal(native_handle_type signum, OnSignal &&callback) {
   using std::forward;

   class impl final : public signal_handler {
      OnSignal on_signal;

   public:
      explicit inline impl(OnSignal &&f) :
         signal_handler(),
         on_signal(forward<OnSignal>(f))
      {
      }

      virtual void handle(const ::signalfd_siginfo &signal_info) final override {
         on_signal(signal_info);
      }
   };

   if (handlers.size() > signum) {
      monitor_signal(signum);
      handlers[signum] = std::make_unique<impl>(std::forward<OnSignal>(callback));
   }
   else {
      throw std::system_error(make_error_code(std::errc::invalid_argument),
                              "Callback cannot be used for invalid signal number.");
   }
}

}

#endif
