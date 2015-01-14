// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_SIGNAL_MANAGER_H__
#define EPOLLING_SIGNAL_MANAGER_H__

#include "event_handle.h"
#include <array>
#include <climits>
#include <csignal>
#include <memory>
#include <system_error>

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
   ~signal_manager() noexcept;

   template<class OnSignal>
   inline std::error_code on_signal(int signum, OnSignal &&callback);
   void stop_monitoring_signal(int signum);

private:
   ::sigset_t signals;
   std::weak_ptr<event_engine> engine;
   // Most likely around 1024 handlers.
   std::array<std::unique_ptr<signal_handler>, sizeof(::sigset_t) * CHAR_BIT> handlers;
   event_handle handle;

   std::error_code monitor_signal(native_handle_type signum) noexcept;
   std::error_code register_with_engine(const signal_manager *self) noexcept;
   void on_signal_triggered();
};


template<class OnSignal>
inline std::error_code signal_manager::on_signal(int signum, OnSignal &&callback) {
   using std::errc;
   using std::error_code;
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

   error_code error;

   if ((0 < signum) && (static_cast<std::size_t>(signum) < handlers.size())) {
      error = monitor_signal(signum);
      if (!error) {
         try {
            handlers[signum] = make_unique<impl>(forward<OnSignal>(callback));
         }
         catch (const std::bad_alloc &) {
            error = make_error_code(errc::not_enough_memory);
         }
      }
   }
   else {
      error = make_error_code(errc::invalid_argument);
   }

   return error;
}

}

#endif
