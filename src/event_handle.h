// vim: sw=3 ts=3 expandtab cindent
#ifndef EPOLLING_EVENT_HANDLE_H__
#define EPOLLING_EVENT_HANDLE_H__
#include <system_error>

namespace epolling {

enum class mode {
   read  = 0x01,
   write = 0x02,
   urgent_read = 0x05,
   one_time = 0x08,
   read_write = read | write,
   urgent_read_write = urgent_read | write
};

class event_engine;

typedef int native_handle_type;

class event_handle {
public:
   virtual ~event_handle() noexcept;

   inline native_handle_type native_handle() const {
      return handle;
   }

   virtual void on_read() = 0;
   virtual void on_write() = 0;
   virtual void on_error() = 0;

protected:
   explicit event_handle(event_engine &e);
   void register_handle(native_handle_type new_handle, mode flags);
   void close_handle();

private:
   event_engine &engine;
   native_handle_type handle;
};

}

#endif
