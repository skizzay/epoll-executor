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

class event_engine;

class signal_manager final {
   friend class event_engine;

   class signal_handler {
   public:
      virtual void handle(const ::signalfd_siginfo &signal_info) = 0;
   };

public:
   explicit signal_manager(event_engine &e);
   ~signal_manager();

   template<class OnSignal>
   inline void on_signal(int signum, OnSignal &&callback);
   void stop_monitoring_signal(int signum);

private:
   ::sigset_t signals;
   std::weak_ptr<event_engine> engine;
   // Most likely around 1024 handlers.
   std::array<std::unique_ptr<signal_handler>, sizeof(::sigset_t) * CHAR_BIT> handlers;
   event_handle handle;

   void monitor_signal(native_handle_type signum);
   void register_with_engine(const signal_manager *self);
   void on_signal_triggered();
};


template<class OnSignal>
inline void signal_manager::on_signal(int signum, OnSignal &&callback) {
   using std::forward;
   using std::make_error_code;
   using std::make_unique;

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

   if ((0 < signum) && (static_cast<std::size_t>(signum) < handlers.size())) {
      monitor_signal(signum);
      handlers[signum] = make_unique<impl>(forward<OnSignal>(callback));
   }
   else {
      throw std::system_error(make_error_code(std::errc::invalid_argument),
                              "Callback cannot be used for invalid signal number.");
   }
}

}

#endif
