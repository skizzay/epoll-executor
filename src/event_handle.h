// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_HANDLE_H__
#define EPOLLING_EVENT_HANDLE_H__

#include "mode.h"
#include <memory>
#include <experimental/string_view>
#include <system_error>
#include <tuple>
#include <utility>

namespace epolling {

typedef int native_handle_type;

class event_handle final {
   class event_trigger {
   public:
      virtual void execute(mode event_flags) = 0;
   };

public:
   template<class Callback>
   event_handle(Callback &&callback) :
      handle(-1),
      trigger()
   {
      using std::forward;
      using std::make_unique;

      class basic_event_trigger final : public event_trigger {
         Callback callback;

      public:
         explicit inline basic_event_trigger(Callback &&f) :
            event_trigger(),
            callback(forward<Callback>(f))
         {
         }

         virtual void execute(mode event_flags) final override {
            (void)callback(event_flags);
         }
      };

      // Can throw std::bad_alloc.
      trigger = make_unique<basic_event_trigger>(forward<Callback>(callback));
   }
   inline event_handle() noexcept :
      handle(-1),
      trigger()
   {
   }
   event_handle(const event_handle &) = delete;
   event_handle(event_handle &&);
   ~event_handle() noexcept;

   event_handle& operator =(const event_handle &) = delete;
   event_handle& operator =(event_handle &&);

   inline native_handle_type native_handle() const noexcept {
      return handle;
   }

   inline bool opened() const noexcept {
      return !closed();
   }
   inline bool closed() const noexcept {
      return 0 > native_handle();
   }

   std::tuple<std::size_t, std::error_code> bytes_available() const noexcept;

   std::error_code make_non_blocking() noexcept;
   std::error_code make_blocking() noexcept;
   std::tuple<bool, std::error_code> is_blocking() const noexcept;

   std::tuple<int, std::error_code> read(void *data, std::size_t num_bytes) noexcept;

   std::tuple<int, std::error_code> write(const void *data, std::size_t num_bytes) noexcept;

   std::error_code open(native_handle_type handle) noexcept;

   std::error_code close() noexcept;

   inline void on_trigger(mode trigger_flags) {
      if (trigger != nullptr) {
         trigger->execute(trigger_flags);
      }
   }

   // NOTE: If you are receiving asynchronous callbacks, then those callbacks will still occur
   //       on the handle that passed into event_engine::start_monitoring.  You must stop
   //       monitoring first, swap, then restart the monitoring.  Synchronous invocations
   //       won't cause any issues.
   inline void swap(event_handle &other) noexcept {
      using std::swap;

      swap(handle, other.handle);
      swap(trigger, other.trigger);
   }

private:
   native_handle_type handle;
   std::unique_ptr<event_trigger> trigger;
};


inline void swap(event_handle &l, event_handle &r) {
   l.swap(r);
}


inline bool operator< (const event_handle &l , const event_handle &r) {
   return l.native_handle() < r.native_handle();
}


inline bool operator==(const event_handle &l , const event_handle &r) {
   return l.native_handle() == r.native_handle();
}


using std::rel_ops::operator!=;
using std::rel_ops::operator<=;
using std::rel_ops::operator>=;
using std::rel_ops::operator>;

}

#endif
