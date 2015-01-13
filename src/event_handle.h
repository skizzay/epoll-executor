// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_HANDLE_H__
#define EPOLLING_EVENT_HANDLE_H__

#include "mode.h"
#include <memory>
#include <experimental/string_view>
#include <system_error>
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

      trigger = make_unique<basic_event_trigger>(forward<Callback>(callback));
   }
   event_handle() = delete;
   event_handle(const event_handle &) = delete;
   event_handle(event_handle &&);
   ~event_handle() noexcept;

   event_handle& operator =(const event_handle &) = delete;
   event_handle& operator =(event_handle &&);

   inline native_handle_type native_handle() const {
      return handle;
   }

   inline bool opened() const {
      return !closed();
   }
   inline bool closed() const {
      return 0 > native_handle();
   }

   int read(void *data, std::size_t num_bytes, std::experimental::string_view what="Failed to read from handle");
   int read(void *data, std::size_t num_bytes, std::error_code &ec) noexcept;

   int write(const void *data, std::size_t num_bytes, std::experimental::string_view what="Failed to write to handle");
   int write(const void *data, std::size_t num_bytes, std::error_code &ec) noexcept;

   void open(native_handle_type handle, std::experimental::string_view what="Failed to open handle");
   void open(native_handle_type handle, std::error_code &ec);

   void close(std::experimental::string_view what="Failed to close handle");
   void close(std::error_code &ec) noexcept;

   inline void on_trigger(mode trigger_flags) {
      trigger->execute(trigger_flags);
   }

   inline void swap(event_handle &other) {
      using std::swap;

      swap(handle, other.handle);
      swap(trigger, other.trigger);
   }

private:
   native_handle_type handle;
   std::unique_ptr<event_trigger> trigger;
};


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
