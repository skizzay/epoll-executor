// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_NOTIFICATION_H__
#define EPOLLING_NOTIFICATION_H__

#include "mode.h"
#include "unique_handle.h"
#include <cstdint>
#include <unistd.h>

namespace epolling {

template<class, template<class> class> class event_engine;


class notification final {
   struct tag {};

public:
   typedef handle<tag, int, -1> handle_type;

   enum class behavior {
      conditional,
      semaphore
   };

   template<class ES, template<class> class D>
   notification(event_engine<ES, D> &e, behavior how_to_behave, uint64_t initial_value);
   notification() = delete;
   notification(notification &&) = default;
   notification& operator =(notification &&) = default;
   notification(const notification &) = delete;
   notification& operator =(const notification &) = delete;

   void set(uint64_t value);
   inline uint64_t get() const {
      return last_read_value;
   }

private:
   static native_handle_type create_native_handle(epolling::notification::behavior behavior, uint64_t initial_value);
   void on_activation(mode activation_flags);

   uint64_t last_read_value;
   unique_handle<handle_type, native_handle_type (*)(native_handle_type)> event_fd;
};

}


#include "event_engine.h"

namespace epolling {

template<class ES, template<class> class D>
inline notification::notification(event_engine<ES, D> &engine, behavior how_to_behave, uint64_t initial_value) :
   last_read_value(std::numeric_limits<uint64_t>::max()),
   event_fd({}, &::close)
{
   event_fd.reset(create_native_handle(how_to_behave, initial_value));
   engine.template start_monitoring<notification, &notification::on_activation>(event_fd.get_handle(), mode::urgent_read, *this);
}

}

#endif
