// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_NOTIFICATION_H__
#define EPOLLING_NOTIFICATION_H__

#include "event_handle.h"
#include <cstdint>

namespace epolling {

class event_engine;

class notification final {
public:
   enum class behavior {
      conditional,
      semaphore
   };

   notification(event_engine &e, behavior how_to_behave, uint64_t initial_value);
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
   uint64_t last_read_value;
   event_handle handle;
};

}

#endif
